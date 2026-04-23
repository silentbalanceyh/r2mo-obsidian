#include "sessionscanner.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDirIterator>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <algorithm>
#include <QTimeZone>

#ifdef Q_OS_MAC
#include <libproc.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#endif

QHash<qint64, quint64> SessionScanner::s_prevCpuTicks;
QHash<qint64, qint64> SessionScanner::s_prevSampleMs;
QHash<qint64, QDateTime> SessionScanner::s_lastWorkingTime;
QHash<qint64, int> SessionScanner::s_highActivitySamples;

SessionScanner::SessionScanner()
{
}

namespace {
QString normalizePath(const QString& path)
{
    return path.isEmpty() ? QString() : QDir::cleanPath(path);
}

bool pathBelongsToProject(const QString& candidatePath, const QString& projectPath)
{
    const QString normalizedCandidate = normalizePath(candidatePath);
    const QString normalizedProject = normalizePath(projectPath);
    if (normalizedCandidate.isEmpty() || normalizedProject.isEmpty()) {
        return false;
    }
    return normalizedCandidate == normalizedProject ||
           normalizedCandidate.startsWith(normalizedProject + "/");
}

bool hasKnownSessionId(const QString& sessionId)
{
    return !sessionId.isEmpty() && sessionId != "unknown";
}

bool isBetterSessionCandidate(const SessionInfo& candidate, const SessionInfo& current)
{
    if (hasKnownSessionId(candidate.sessionId) != hasKnownSessionId(current.sessionId)) {
        return hasKnownSessionId(candidate.sessionId);
    }
    if (candidate.sessionPath.isEmpty() != current.sessionPath.isEmpty()) {
        return !candidate.sessionPath.isEmpty();
    }
    if (candidate.toolName == "Codex") {
        const bool candidateIsVendor = candidate.detailText.contains("/vendor/");
        const bool currentIsVendor = current.detailText.contains("/vendor/");
        if (candidateIsVendor != currentIsVendor) {
            return candidateIsVendor;
        }
    }
    if (candidate.status != current.status) {
        return candidate.status == SessionStatus::Working;
    }
    const bool candidateRunning = candidate.status == SessionStatus::Working;
    const bool currentRunning = current.status == SessionStatus::Working;
    if (candidateRunning != currentRunning) {
        return candidateRunning;
    }
    return candidate.processPid < current.processPid;
}

QStringList extractUniqueOpenCodeSessionIds(const QString& rawContent)
{
    QStringList sessionIds;
    QSet<QString> seen;
    QRegularExpression re(R"((ses_[A-Za-z0-9]+))");
    QRegularExpressionMatchIterator it = re.globalMatch(rawContent);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString sessionId = match.captured(1);
        if (!sessionId.isEmpty() && !seen.contains(sessionId)) {
            seen.insert(sessionId);
            sessionIds.append(sessionId);
        }
    }
    return sessionIds;
}

QByteArray trimmedJsonLine(const QByteArray& line)
{
    const QByteArray trimmed = line.trimmed();
    if (trimmed.isEmpty() || (!trimmed.startsWith('{') && !trimmed.startsWith('['))) {
        return QByteArray();
    }
    return trimmed;
}

QJsonObject parseLastJsonObject(const QByteArray& tail)
{
    const QList<QByteArray> lines = tail.split('\n');
    for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
        const QByteArray candidate = trimmedJsonLine(*it);
        if (candidate.isEmpty()) {
            continue;
        }
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(candidate, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            return doc.object();
        }
    }
    return QJsonObject();
}

QList<QJsonObject> parseRecentJsonObjects(const QByteArray& tail, int maxObjects = 96)
{
    QList<QJsonObject> objects;
    const QList<QByteArray> lines = tail.split('\n');
    for (auto it = lines.crbegin(); it != lines.crend() && objects.size() < maxObjects; ++it) {
        const QByteArray candidate = trimmedJsonLine(*it);
        if (candidate.isEmpty()) {
            continue;
        }
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(candidate, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            objects.append(doc.object());
        }
    }
    return objects;
}

QDateTime parseArtifactEventTime(const QJsonObject& object)
{
    const QStringList keys = {
        QStringLiteral("timestamp"),
        QStringLiteral("time"),
        QStringLiteral("created_at")
    };
    for (const QString& key : keys) {
        const QString raw = object.value(key).toString();
        if (raw.isEmpty()) {
            continue;
        }
        const QDateTime parsed = QDateTime::fromString(raw, Qt::ISODateWithMs);
        if (parsed.isValid()) {
            return parsed.toUTC();
        }
        const QDateTime parsedFallback = QDateTime::fromString(raw, Qt::ISODate);
        if (parsedFallback.isValid()) {
            return parsedFallback.toUTC();
        }
    }
    return QDateTime();
}

QDateTime parseUnixTimestampSeconds(qint64 value)
{
    if (value <= 0) {
        return QDateTime();
    }
    return QDateTime::fromSecsSinceEpoch(value, QTimeZone::UTC);
}

bool workspaceTerminalMentionsProject(const QString& rawContent, const QString& projectPath)
{
    const QString normalizedPath = QDir::cleanPath(projectPath);
    const QString projectName = QFileInfo(normalizedPath).fileName();
    if (!normalizedPath.isEmpty() && rawContent.contains(normalizedPath, Qt::CaseInsensitive)) {
        return true;
    }
    if (!projectName.isEmpty()) {
        const QStringList hints = {
            QString("/%1").arg(projectName),
            QString("➜  %1").arg(projectName),
            QString(" %1 ").arg(projectName),
            projectName
        };
        for (const QString& hint : hints) {
            if (rawContent.contains(hint, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

struct OpenCodeDatabaseSession {
    QString projectPath;
    QString directory;
    QString sessionId;
};

struct CodexLogSessionRecord {
    QString sessionId;
    QString projectPath;
    QDateTime lastSeenAt;
};

QList<OpenCodeDatabaseSession> collectOpenCodeSessionsFromDatabase(const QString& projectPath)
{
    QList<OpenCodeDatabaseSession> sessions;
    const QString normalizedProjectPath = normalizePath(projectPath);
    if (normalizedProjectPath.isEmpty()) {
        return sessions;
    }

    const QString databasePath = QDir::homePath() + "/.local/share/opencode/opencode.db";
    if (!QFileInfo::exists(databasePath)) {
        return sessions;
    }

    const QString query = QStringLiteral(
        "select p.worktree, s.directory, s.id "
        "from session s "
        "join project p on p.id = s.project_id "
        "where s.time_archived is null "
        "order by s.time_updated desc;");

    QProcess sqlite;
    sqlite.start("sqlite3", QStringList() << "-noheader" << "-separator" << "|" << databasePath << query);
    if (!sqlite.waitForFinished(3000) ||
        sqlite.exitStatus() != QProcess::NormalExit ||
        sqlite.exitCode() != 0) {
        sqlite.kill();
        return sessions;
    }

    QSet<QString> seen;
    const QStringList lines = QString::fromUtf8(sqlite.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList parts = line.split('|');
        if (parts.size() < 3) {
            continue;
        }

        const QString worktree = normalizePath(parts.at(0).trimmed());
        const QString directory = normalizePath(parts.at(1).trimmed());
        const QString sessionId = parts.at(2).trimmed();
        if (sessionId.isEmpty() || seen.contains(sessionId)) {
            continue;
        }

        const bool matchesWorktree =
            pathBelongsToProject(worktree, normalizedProjectPath) ||
            pathBelongsToProject(normalizedProjectPath, worktree);
        const bool matchesDirectory =
            pathBelongsToProject(directory, normalizedProjectPath) ||
            pathBelongsToProject(normalizedProjectPath, directory);
        if (!matchesWorktree && !matchesDirectory) {
            continue;
        }

        seen.insert(sessionId);
        sessions.append(OpenCodeDatabaseSession{worktree, directory, sessionId});
    }

    return sessions;
}

QStringList collectOpenCodeSessionIdsFromDatabase(const QString& projectPath)
{
    QStringList sessionIds;
    const QList<OpenCodeDatabaseSession> sessions = collectOpenCodeSessionsFromDatabase(projectPath);
    for (const OpenCodeDatabaseSession& session : sessions) {
        sessionIds.append(session.sessionId);
    }
    return sessionIds;
}

QString sessionIdFromRolloutBaseName(const QString& base)
{
    const QStringList parts = base.split('-');
    return parts.size() >= 5 ? parts.mid(parts.size() - 5).join("-") : base;
}

QJsonObject parseFirstJsonObject(const QByteArray& head)
{
    const QList<QByteArray> lines = head.split('\n');
    for (const QByteArray& line : lines) {
        const QByteArray candidate = trimmedJsonLine(line);
        if (candidate.isEmpty()) {
            continue;
        }
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(candidate, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            return doc.object();
        }
    }
    return QJsonObject();
}

QDateTime parseLogTimestamp(const QString& raw)
{
    if (raw.isEmpty()) {
        return QDateTime();
    }

    QDateTime parsed = QDateTime::fromString(raw, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(raw, Qt::ISODate);
    }
    return parsed.isValid() ? parsed.toUTC() : QDateTime();
}

QString extractJsonStringField(const QString& line, const QString& fieldName)
{
    const QRegularExpression re(
        QStringLiteral("\"%1\":\"([^\"]+)\"").arg(QRegularExpression::escape(fieldName)));
    const QRegularExpressionMatch match = re.match(line);
    return match.hasMatch() ? match.captured(1) : QString();
}

QList<CodexLogSessionRecord> collectCodexSessionLogRecords()
{
    static QDateTime s_cachedModifiedAt;
    static qint64 s_cachedSize = -1;
    static QList<CodexLogSessionRecord> s_cachedRecords;

    const QString logPath = QDir::homePath() + "/.codex/log/codex-tui.log";
    const QFileInfo logInfo(logPath);
    if (!logInfo.exists() || !logInfo.isFile()) {
        return {};
    }

    if (s_cachedModifiedAt == logInfo.lastModified() &&
        s_cachedSize == logInfo.size()) {
        return s_cachedRecords;
    }

    QFile file(logPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    static constexpr qint64 maxBytes = 16 * 1024 * 1024;
    const qint64 size = file.size();
    if (size > maxBytes) {
        file.seek(size - maxBytes);
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);

    QHash<QString, CodexLogSessionRecord> latestByKey;
    const QRegularExpression threadRe(
        QStringLiteral("(?:thread_id|thread\\.id)=([0-9a-f]{8,}-[0-9a-f-]{20,})"),
        QRegularExpression::CaseInsensitiveOption);

    for (const QString& rawLine : lines) {
        if (!rawLine.contains("thread_id=", Qt::CaseInsensitive) &&
            !rawLine.contains("thread.id=", Qt::CaseInsensitive)) {
            continue;
        }

        const QRegularExpressionMatch threadMatch = threadRe.match(rawLine);
        if (!threadMatch.hasMatch()) {
            continue;
        }

        QString projectPath = extractJsonStringField(rawLine, QStringLiteral("workdir"));
        if (projectPath.isEmpty()) {
            projectPath = extractJsonStringField(rawLine, QStringLiteral("cwd"));
        }
        projectPath = normalizePath(projectPath);
        if (projectPath.isEmpty()) {
            continue;
        }

        CodexLogSessionRecord record;
        record.sessionId = threadMatch.captured(1);
        record.projectPath = projectPath;
        record.lastSeenAt = parseLogTimestamp(rawLine.section(' ', 0, 0));

        const QString key = record.sessionId + "|" + record.projectPath;
        const auto existing = latestByKey.constFind(key);
        if (existing == latestByKey.cend() || existing->lastSeenAt < record.lastSeenAt) {
            latestByKey.insert(key, record);
        }
    }

    QList<CodexLogSessionRecord> records = latestByKey.values();
    std::sort(records.begin(), records.end(), [](const CodexLogSessionRecord& lhs,
                                                 const CodexLogSessionRecord& rhs) {
        return lhs.lastSeenAt > rhs.lastSeenAt;
    });

    s_cachedModifiedAt = logInfo.lastModified();
    s_cachedSize = logInfo.size();
    s_cachedRecords = records;
    return s_cachedRecords;
}

QList<CodexLogSessionRecord> collectCodexLogCandidatesForProject(
    const QList<CodexLogSessionRecord>& records,
    const QString& projectPath,
    const QSet<QString>& usedSessionIds)
{
    QList<CodexLogSessionRecord> exactMatches;
    QList<CodexLogSessionRecord> relatedMatches;
    const QString normalizedProjectPath = normalizePath(projectPath);

    for (const CodexLogSessionRecord& record : records) {
        if (usedSessionIds.contains(record.sessionId)) {
            continue;
        }
        if (normalizedProjectPath.isEmpty() || record.projectPath.isEmpty()) {
            continue;
        }
        if (record.projectPath == normalizedProjectPath) {
            exactMatches.append(record);
            continue;
        }
        if (pathBelongsToProject(record.projectPath, normalizedProjectPath) ||
            pathBelongsToProject(normalizedProjectPath, record.projectPath)) {
            relatedMatches.append(record);
        }
    }

    exactMatches.append(relatedMatches);
    return exactMatches;
}
}

QString SessionScanner::toolIconPath(const QString& toolName)
{
    QString lower = toolName.toLower();
    if (lower.contains("claude")) return ":/ai-tools/ai-tools/claude.png";
    if (lower.contains("codex")) return ":/ai-tools/ai-tools/codex.png";
    if (lower.contains("opencode")) return ":/ai-tools/ai-tools/opencode.png";
    if (lower.contains("cursor")) return ":/ai-tools/ai-tools/cursor.png";
    if (lower.contains("cherry") || lower.contains("trae")) return ":/ai-tools/ai-tools/cherry.png";
    return QString();
}

QString SessionScanner::terminalIconPath(const QString& terminalName)
{
    QString lower = terminalName.toLower();
    if (lower.contains("ssh")) return ":/icons/ui/terminal-ubuntu.svg";
    if (lower.contains("wezterm")) {
        if (QFile::exists(QStringLiteral("/Applications/WezTerm.app"))) {
            return ":/icons/ui/terminal-wezterm.png";
        }
        return ":/icons/ui/terminal-wezterm.svg";
    }
    if (lower.contains("iterm")) {
        if (QFile::exists(QStringLiteral("/Applications/iTerm.app"))) {
            return ":/icons/ui/terminal-iterm.png";
        }
        return ":/icons/ui/terminal-iterm.svg";
    }
    if (lower.contains("ghostty")) {
        if (QFile::exists(QStringLiteral("/Applications/Ghostty.app"))) {
            return ":/icons/ui/terminal-ghostty.png";
        }
        return ":/icons/ui/terminal-macos.svg";
    }
    if (lower.contains("terminal")) {
        if (QFile::exists(QStringLiteral("/Applications/Utilities/Terminal.app"))) {
            return ":/icons/ui/terminal-terminalapp.png";
        }
        if (QFile::exists(QStringLiteral("/System/Applications/Utilities/Terminal.app"))) {
            return ":/icons/ui/terminal-terminalapp.png";
        }
        return ":/icons/ui/terminal-macos.svg";
    }
    return QString();
}

QStringList SessionScanner::getChildPids(qint64 parentPid) const
{
    QStringList result;
#ifdef Q_OS_MAC
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size = 0;
    if (sysctl(mib, 4, nullptr, &size, nullptr, 0) < 0) return result;

    struct kinfo_proc *procs = (struct kinfo_proc *)malloc(size);
    if (!procs) return result;

    if (sysctl(mib, 4, procs, &size, nullptr, 0) < 0) {
        free(procs);
        return result;
    }

    int count = (int)(size / sizeof(struct kinfo_proc));
    for (int i = 0; i < count; ++i) {
        if (procs[i].kp_eproc.e_ppid == parentPid) {
            result.append(QString::number(procs[i].kp_proc.p_pid));
        }
    }
    free(procs);
#else
    Q_UNUSED(parentPid);
#endif
    return result;
}

namespace {
QString terminalLabelFromCommand(const QString& cmd)
{
    const QString lower = cmd.toLower();
    if (lower.contains("wezterm")) return QStringLiteral("WezTerm");
    if (lower.contains("iterm")) return QStringLiteral("iTerm2");
    if (lower.contains("ghostty")) return QStringLiteral("Ghostty");
    if (lower.contains("/terminal") || lower.contains("terminal.app")) return QStringLiteral("Terminal.app");
    return QString();
}
}

QStringList SessionScanner::getAllChildPids(qint64 parentPid, int maxDepth) const
{
    QStringList result;
    QList<qint64> queue;
    queue.append(parentPid);
    QSet<qint64> visited;
    visited.insert(parentPid);

    for (int depth = 0; depth < maxDepth && !queue.isEmpty(); ++depth) {
        QList<qint64> nextQueue;
        for (qint64 pid : queue) {
            QStringList children = getChildPids(pid);
            for (const QString& childStr : children) {
                qint64 childPid = childStr.toLongLong();
                if (!visited.contains(childPid)) {
                    visited.insert(childPid);
                    result.append(childStr);
                    nextQueue.append(childPid);
                }
            }
        }
        queue = nextQueue;
    }
    return result;
}

QString SessionScanner::getProcessCommand(qint64 pid) const
{
#ifdef Q_OS_MAC
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) > 0) {
        return QString::fromUtf8(pathbuf);
    }
#else
    Q_UNUSED(pid);
#endif
    return QString();
}

bool SessionScanner::isShellProcess(const QString& cmd) const
{
    const QString lower = cmd.toLower();
    return lower.contains("zsh") || lower.contains("bash") || lower.contains("fish");
}

QString SessionScanner::getProcessWorkingDir(qint64 pid) const
{
#ifdef Q_OS_MAC
    struct proc_vnodepathinfo vpi;
    int ret = proc_pidinfo(pid, PROC_PIDVNODEPATHINFO, 0, &vpi, sizeof(vpi));
    if (ret > 0) {
        return QString::fromUtf8(vpi.pvi_cdir.vip_path);
    }
#else
    Q_UNUSED(pid);
#endif
    return QString();
}

qint64 SessionScanner::getParentPid(qint64 pid) const
{
#ifdef Q_OS_MAC
    struct proc_bsdinfo bsdInfo;
    const int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsdInfo, sizeof(bsdInfo));
    if (ret > 0) {
        return static_cast<qint64>(bsdInfo.pbi_ppid);
    }
#else
    Q_UNUSED(pid);
#endif
    return 0;
}

QDateTime SessionScanner::getProcessStartedAt(qint64 pid) const
{
    QProcess proc;
    proc.start("ps", QStringList() << "-p" << QString::number(pid) << "-o" << "lstart=");
    if (!proc.waitForFinished(1000) ||
        proc.exitStatus() != QProcess::NormalExit ||
        proc.exitCode() != 0) {
        proc.kill();
        return QDateTime();
    }

    const QString raw = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (raw.isEmpty()) {
        return QDateTime();
    }

    QLocale locale = QLocale::c();
    QDateTime startedAt = locale.toDateTime(raw, QStringLiteral("ddd MMM d HH:mm:ss yyyy"));
    if (!startedAt.isValid()) {
        startedAt = locale.toDateTime(raw, QStringLiteral("ddd MMM dd HH:mm:ss yyyy"));
    }
    if (!startedAt.isValid()) {
        return QDateTime();
    }
    startedAt.setTimeZone(QTimeZone::systemTimeZone());
    return startedAt.toUTC();
    return QDateTime();
}

bool SessionScanner::isProcessRunning(qint64 pid) const
{
#ifdef Q_OS_MAC
    struct proc_bsdinfo bsdInfo;
    const int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsdInfo, sizeof(bsdInfo));
    if (ret > 0) {
        const char state = static_cast<char>(bsdInfo.pbi_status);
        return state == SRUN;
    }
#else
    Q_UNUSED(pid);
#endif
    return false;
}

quint64 SessionScanner::getProcessCpuTicks(qint64 pid) const
{
#ifdef Q_OS_MAC
    struct proc_taskinfo pti;
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
    if (ret > 0) {
        return pti.pti_total_user + pti.pti_total_system;
    }
#else
    Q_UNUSED(pid);
#endif
    return 0;
}

QByteArray SessionScanner::readArtifactTail(const QString& sessionPath, qint64 maxBytes) const
{
    QFile file(sessionPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    const qint64 size = file.size();
    if (size > maxBytes) {
        file.seek(size - maxBytes);
    }
    return file.readAll();
}

SessionStatus SessionScanner::inferCodexArtifactStatus(const QString& sessionPath) const
{
    const double codexActiveFreshSeconds = 8.0;
    const QByteArray tail = readArtifactTail(sessionPath);
    if (tail.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QList<QJsonObject> objects = parseRecentJsonObjects(tail);
    if (objects.isEmpty()) {
        return SessionStatus::Unknown;
    }

    for (const QJsonObject& object : objects) {
        const QString type = object.value("type").toString();
        const QDateTime eventTime = parseArtifactEventTime(object);
        const bool eventIsFresh = eventTime.isValid() &&
            eventTime.secsTo(QDateTime::currentDateTimeUtc()) <= codexActiveFreshSeconds;
        if (type == "task_complete") {
            return SessionStatus::Ready;
        }
        if (type == "task_started") {
            return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
        }

        if (type == "event_msg") {
            const QJsonObject payload = object.value("payload").toObject();
            const QString payloadType = payload.value("type").toString();
            if (payloadType == "user_message") {
                return SessionStatus::Ready;
            }
            if (payloadType == "exec_command_begin" ||
                payloadType == "function_call_begin" ||
                payloadType == "patch_apply_begin" ||
                payloadType == "agent_reasoning") {
                return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
            }
            if (payloadType == "exec_command_end" ||
                payloadType == "function_call_output" ||
                payloadType == "patch_apply_end") {
                return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
            }
            continue;
        }

        if (type == "response_item") {
            const QJsonObject payload = object.value("payload").toObject();
            const QString payloadType = payload.value("type").toString();
            if (payloadType == "function_call" ||
                payloadType == "custom_tool_call" ||
                payloadType == "reasoning") {
                return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
            }
            if (payloadType == "function_call_output") {
                return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
            }
            if (payloadType == "message") {
                const QString phase = payload.value("phase").toString();
                if (phase.isEmpty() || phase == "final_answer") {
                    return SessionStatus::Ready;
                }
                if (phase == "commentary") {
                    return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
                }
            }
        }
    }

    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::inferClaudeArtifactStatus(const QString& sessionPath) const
{
    const double claudeActiveFreshSeconds = 8.0;
    const double claudeReadyFreshSeconds = 45.0;
    const QByteArray tail = readArtifactTail(sessionPath);
    if (tail.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QFileInfo sessionFile(sessionPath);
    const QDateTime artifactModifiedAt = sessionFile.exists() && sessionFile.lastModified().isValid()
        ? sessionFile.lastModified().toUTC()
        : QDateTime();

    const QList<QJsonObject> objects = parseRecentJsonObjects(tail);
    if (objects.isEmpty()) {
        return SessionStatus::Unknown;
    }

    for (const QJsonObject& object : objects) {
        const QString type = object.value("type").toString();
        const QString subtype = object.value("subtype").toString();
        const QDateTime eventTime = parseArtifactEventTime(object);
        const QDateTime freshBaseTime = eventTime.isValid() ? eventTime : artifactModifiedAt;
        const bool eventIsFresh = eventTime.isValid() &&
            eventTime.secsTo(QDateTime::currentDateTimeUtc()) <= claudeActiveFreshSeconds;
        const bool readyStateIsFresh = freshBaseTime.isValid() &&
            freshBaseTime.secsTo(QDateTime::currentDateTimeUtc()) <= claudeReadyFreshSeconds;
        if (subtype == "stop_hook_summary") {
            return readyStateIsFresh ? SessionStatus::Ready : SessionStatus::Unknown;
        }

        if (type == "attachment") {
            const QJsonObject attachment = object.value("attachment").toObject();
            const QString hookEvent = attachment.value("hookEvent").toString();
            const QString attachmentType = attachment.value("type").toString();
            if (hookEvent == "PreToolUse" || hookEvent == "PostToolUse") {
                return SessionStatus::Working;
            }
            if (hookEvent == "Stop" ||
                attachmentType == "hook_success" ||
                attachmentType == "async_hook_response") {
                return readyStateIsFresh ? SessionStatus::Ready : SessionStatus::Unknown;
            }
            continue;
        }

        if (type == "progress") {
            const QJsonObject data = object.value("data").toObject();
            const QString hookEvent = data.value("hookEvent").toString();
            if (hookEvent == "PreToolUse" || hookEvent == "PostToolUse") {
                return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
            }
            if (hookEvent == "Stop") {
                return readyStateIsFresh ? SessionStatus::Ready : SessionStatus::Unknown;
            }
            continue;
        }

        if (type == "assistant") {
            const QJsonObject message = object.value("message").toObject();
            const QString stopReason = message.value("stop_reason").toString();
            if (stopReason == "tool_use") {
                return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
            }
            if (stopReason == "end_turn") {
                return readyStateIsFresh ? SessionStatus::Ready : SessionStatus::Unknown;
            }
            continue;
        }

        if (type == "system") {
            if (subtype == "turn_duration" || subtype == "away_summary") {
                return readyStateIsFresh ? SessionStatus::Ready : SessionStatus::Unknown;
            }
            continue;
        }

        if (type == "user" ||
            type == "last-prompt" ||
            type == "file-history-snapshot" ||
            type == "permission-mode") {
            return readyStateIsFresh ? SessionStatus::Ready : SessionStatus::Unknown;
        }

        if (type == "tool_use") {
            return eventIsFresh ? SessionStatus::Working : SessionStatus::Ready;
        }
    }

    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::inferOpenCodeArtifactStatus(const QString& sessionId,
                                                          const QString& sessionPath) const
{
    const QByteArray tail = readArtifactTail(sessionPath, 131072);
    if (tail.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QString text = QString::fromUtf8(tail);
    if (!sessionId.isEmpty() &&
        sessionId != "unknown" &&
        text.contains(QString("\"paused\":{\"%1\":true").arg(sessionId))) {
        return SessionStatus::Ready;
    }
    if (text.contains("\"workspace:followup\"") &&
        text.contains("\"items\":{},\"failed\":{},\"edit\":{}") &&
        (!text.contains("\"paused\":{") || text.contains("\"paused\":{}"))) {
        return SessionStatus::Ready;
    }

    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::inferOpenCodeGlobalStatus(const QString& sessionId) const
{
    if (sessionId.isEmpty() || sessionId == "unknown") {
        return SessionStatus::Unknown;
    }

    const double openCodeActiveFreshSeconds = 8.0;
    const QString globalPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + "/Library/Application Support/ai.opencode.desktop/opencode.global.dat";
    const QByteArray tail = readArtifactTail(globalPath, 262144);
    if (tail.isEmpty()) {
        return SessionStatus::Unknown;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(tail, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return SessionStatus::Unknown;
    }

    QJsonParseError notificationParseError;
    const QString notificationText = doc.object().value("notification").toString();
    if (notificationText.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QJsonDocument notificationDoc = QJsonDocument::fromJson(notificationText.toUtf8(), &notificationParseError);
    if (notificationParseError.error != QJsonParseError::NoError || !notificationDoc.isObject()) {
        return SessionStatus::Unknown;
    }

    const QJsonArray notifications = notificationDoc.object().value("list").toArray();

    for (int i = notifications.size() - 1; i >= 0; --i) {
        const QJsonValue item = notifications.at(i);
        if (!item.isObject()) {
            continue;
        }
        const QJsonObject entry = item.toObject();
        if (entry.value("session").toString() != sessionId) {
            continue;
        }

        const QString type = entry.value("type").toString();
        const qint64 notificationEpochMs = static_cast<qint64>(entry.value("time").toDouble());
        const QDateTime notificationTime = notificationEpochMs > 0
            ? QDateTime::fromMSecsSinceEpoch(notificationEpochMs, Qt::UTC)
            : QDateTime();
        const bool notificationIsFresh = notificationTime.isValid() &&
            notificationTime.msecsTo(QDateTime::currentDateTimeUtc()) <=
                static_cast<qint64>(openCodeActiveFreshSeconds * 1000.0);
        if (type == "turn-start" || type == "session.status") {
            return notificationIsFresh ? SessionStatus::Working : SessionStatus::Ready;
        }
        if (type == "turn-complete" || type == "error") {
            return SessionStatus::Ready;
        }
        return SessionStatus::Unknown;
    }

    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::inferArtifactStatus(const QString& toolName, const QString& sessionId,
                                                  const QString& sessionPath) const
{
    if (toolName == "OpenCode") {
        const SessionStatus globalStatus = inferOpenCodeGlobalStatus(sessionId);
        if (globalStatus != SessionStatus::Unknown) {
            return globalStatus;
        }
        if (sessionPath.isEmpty()) {
            return SessionStatus::Unknown;
        }
        return inferOpenCodeArtifactStatus(sessionId, sessionPath);
    }

    if (sessionPath.isEmpty()) {
        return SessionStatus::Unknown;
    }

    if (toolName == "Codex") {
        return inferCodexArtifactStatus(sessionPath);
    }
    if (toolName == "Claude") {
        return inferClaudeArtifactStatus(sessionPath);
    }
    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::determineStatus(qint64 pid, quint64 currentTicks, bool isRunning,
                                              const QString& toolName, const QString& sessionId,
                                              const QString& sessionPath) const
{
    const double workingKeepAliveSeconds = 6.0;
    const SessionStatus artifactStatus = inferArtifactStatus(toolName, sessionId, sessionPath);
    if (artifactStatus != SessionStatus::Unknown) {
        if (artifactStatus == SessionStatus::Working) {
            s_lastWorkingTime[pid] = QDateTime::currentDateTime();
        }
        return artifactStatus;
    }

    if (toolName == "OpenCode") {
        return SessionStatus::Ready;
    }

    if (!isRunning) {
        return SessionStatus::Ready;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    quint64 prevTicks = s_prevCpuTicks.value(pid, 0);
    qint64 prevSampleMs = s_prevSampleMs.value(pid, 0);
    s_prevCpuTicks[pid] = currentTicks;
    s_prevSampleMs[pid] = nowMs;

    if (prevTicks == 0 || prevSampleMs == 0) {
        s_highActivitySamples[pid] = 0;
        return SessionStatus::Ready;
    }

    quint64 delta = 0;
    if (currentTicks > prevTicks) {
        delta = currentTicks - prevTicks;
    }

    const qint64 elapsedMs = qMax<qint64>(1, nowMs - prevSampleMs);
    const double deltaPerSecond = (static_cast<double>(delta) * 1000.0) / static_cast<double>(elapsedMs);
    const bool strongCpuBurst = deltaPerSecond >= 300000.0;
    const bool sustainedCpuActivity = deltaPerSecond >= 90000.0;

    if (strongCpuBurst) {
        s_highActivitySamples[pid] = qMax(2, s_highActivitySamples.value(pid, 0) + 1);
        s_lastWorkingTime[pid] = QDateTime::currentDateTime();
        return SessionStatus::Working;
    }

    if (sustainedCpuActivity) {
        const int highSamples = s_highActivitySamples.value(pid, 0) + 1;
        s_highActivitySamples[pid] = highSamples;
        if (highSamples >= 2) {
            s_lastWorkingTime[pid] = QDateTime::currentDateTime();
            return SessionStatus::Working;
        }
    } else {
        s_highActivitySamples[pid] = 0;
    }

    if (s_lastWorkingTime.contains(pid)) {
        qint64 secsSinceWork = s_lastWorkingTime[pid].secsTo(QDateTime::currentDateTime());
        if (secsSinceWork < workingKeepAliveSeconds) {
            return SessionStatus::Working;
        }
    }

    return SessionStatus::Ready;
}

QString SessionScanner::identifyToolName(const QString& cmd) const
{
    QString lower = cmd.toLower();
    if (lower.contains("claude")) return "Claude";
    if (lower.contains("codex")) return "Codex";
    if (lower.contains("opencode")) return "OpenCode";
    if (lower.contains("gemini")) return "Gemini";
    if (lower.contains("aider")) return "Aider";
    if (lower.contains("cursor")) return "Cursor";
    if (lower.contains("trae")) return "Trae";
    return QString();
}

QString SessionScanner::extractSessionIdFromCommand(const QString& cmd, const QString& toolName) const
{
    if (toolName == "Codex") {
        QRegularExpression re(R"((?:^|\s)(?:resume\s+)?([0-9a-f]{8,}-[0-9a-f-]{20,})(?:\s|$))",
                              QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = re.match(cmd);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }

    return QString();
}

void SessionScanner::findLatestClaudeSessionArtifact(const QString& projectPath, QString& sessionId,
                                                     QString& sessionPath) const
{
    const QList<GenericSessionArtifact> artifacts = collectClaudeSessionArtifacts(projectPath);
    if (!artifacts.isEmpty()) {
        sessionId = artifacts.first().sessionId;
        sessionPath = artifacts.first().sessionPath;
    }
}

QList<SessionScanner::GenericSessionArtifact> SessionScanner::collectClaudeSessionArtifacts(const QString& projectPath) const
{
    QList<GenericSessionArtifact> artifacts;
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString escaped = projectPath;
    escaped.replace("/", "-");
    QString projDir = homeDir + "/.claude/projects/" + escaped;
    QDir dir(projDir);
    if (!dir.exists()) {
        dir.setPath(homeDir + "/.claude/projects/-" + escaped);
    }

    if (!dir.exists()) {
        return artifacts;
    }

    QDirIterator it(dir.path(),
                    QStringList() << "*.jsonl" << "*.json",
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        const bool isSubagentArtifact = fi.filePath().contains("/subagents/");
        const QString base = fi.completeBaseName();
        if (base.length() >= 32 || base.count("-") >= 4) {
            GenericSessionArtifact artifact;
            artifact.sessionId = base;
            artifact.sessionPath = fi.filePath();
            artifact.projectPath = normalizePath(projectPath);
            artifact.lastModified = fi.lastModified();
            Q_UNUSED(isSubagentArtifact);
            artifacts.append(artifact);
        }
    }

    std::sort(artifacts.begin(), artifacts.end(), [](const GenericSessionArtifact& lhs, const GenericSessionArtifact& rhs) {
        return lhs.lastModified > rhs.lastModified;
    });
    return artifacts;
}

QList<SessionScanner::CodexSessionArtifact> SessionScanner::collectCodexSessionArtifacts() const
{
    QList<CodexSessionArtifact> artifacts;
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir dir(homeDir + "/.codex/sessions");
    if (!dir.exists()) {
        return artifacts;
    }

    QDirIterator it(dir.path(), QStringList() << "rollout-*.jsonl", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        const QString base = fi.completeBaseName();
        QFile file(fi.filePath());
        QString artifactProjectPath;
        QDateTime artifactStartedAt;
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray head = file.read(8192);
            const QJsonObject firstObject = parseFirstJsonObject(head);
            if (firstObject.value("type").toString() == "session_meta") {
                const QJsonObject payload = firstObject.value("payload").toObject();
                artifactProjectPath = QDir::cleanPath(payload.value("cwd").toString());
                artifactStartedAt = parseArtifactEventTime(firstObject);
                if (!artifactStartedAt.isValid()) {
                    artifactStartedAt = parseArtifactEventTime(payload);
                }
            }
        }

        CodexSessionArtifact artifact;
        artifact.sessionId = sessionIdFromRolloutBaseName(base);
        artifact.sessionPath = fi.filePath();
        artifact.projectPath = artifactProjectPath;
        artifact.lastModified = fi.lastModified();
        artifact.startedAt = artifactStartedAt;
        artifacts.append(artifact);
    }

    std::sort(artifacts.begin(), artifacts.end(), [](const CodexSessionArtifact& lhs, const CodexSessionArtifact& rhs) {
        return lhs.lastModified > rhs.lastModified;
    });
    return artifacts;
}

void SessionScanner::findLatestOpenCodeSessionArtifact(const QString& projectPath, QString& sessionId,
                                                       QString& sessionPath) const
{
    Q_UNUSED(sessionId);
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir dir(homeDir + "/Library/Application Support/ai.opencode.desktop");
    if (!dir.exists()) {
        return;
    }

    const QFileInfoList files = dir.entryInfoList(
        QStringList() << "opencode.workspace*.dat",
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time);

    const QString normalizedProjectPath = normalizePath(projectPath);
    for (const QFileInfo& fileInfo : files) {
        QFile file(fileInfo.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QString rawContent = QString::fromUtf8(file.readAll());
        file.close();

        if (!rawContent.contains("ses_")) {
            continue;
        }

        const QStringList sessionIds = extractUniqueOpenCodeSessionIds(rawContent);
        if (sessionIds.isEmpty()) {
            continue;
        }

        if (!normalizedProjectPath.isEmpty() &&
            workspaceTerminalMentionsProject(rawContent, normalizedProjectPath)) {
            sessionPath = fileInfo.filePath();
            return;
        }
    }
}

QStringList SessionScanner::collectOpenCodeSessionIdsForProject(const QString& projectPath) const
{
    QStringList sessionIds = collectOpenCodeSessionIdsFromDatabase(projectPath);
    QSet<QString> seen(sessionIds.begin(), sessionIds.end());
    const QString normalizedProjectPath = normalizePath(projectPath);
    if (normalizedProjectPath.isEmpty()) {
        return sessionIds;
    }

    const QString globalPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + "/Library/Application Support/ai.opencode.desktop/opencode.global.dat";
    const QByteArray globalTail = readArtifactTail(globalPath, 524288);
    if (!globalTail.isEmpty()) {
        QJsonParseError globalError;
        const QJsonDocument globalDoc = QJsonDocument::fromJson(globalTail, &globalError);
        if (globalError.error == QJsonParseError::NoError && globalDoc.isObject()) {
            const QJsonObject globalObject = globalDoc.object();
            const QString notificationText = globalDoc.object().value("notification").toString();
            if (!notificationText.isEmpty()) {
                QJsonParseError notificationError;
                const QJsonDocument notificationDoc = QJsonDocument::fromJson(notificationText.toUtf8(), &notificationError);
                if (notificationError.error == QJsonParseError::NoError && notificationDoc.isObject()) {
                    const QJsonArray notifications = notificationDoc.object().value("list").toArray();
                    for (int i = notifications.size() - 1; i >= 0; --i) {
                        const QJsonObject entry = notifications.at(i).toObject();
                        const QString directory = normalizePath(entry.value("directory").toString());
                        const QString sessionId = entry.value("session").toString();
                        if (sessionId.isEmpty()) {
                            continue;
                        }
                        if (!pathBelongsToProject(directory, normalizedProjectPath) &&
                            !pathBelongsToProject(normalizedProjectPath, directory)) {
                            continue;
                        }
                        if (!seen.contains(sessionId)) {
                            seen.insert(sessionId);
                            sessionIds.append(sessionId);
                        }
                    }
                }
            }

            const QString layoutPageText = globalObject.value("layout.page").toString();
            if (!layoutPageText.isEmpty()) {
                QJsonParseError layoutError;
                const QJsonDocument layoutDoc = QJsonDocument::fromJson(layoutPageText.toUtf8(), &layoutError);
                if (layoutError.error == QJsonParseError::NoError && layoutDoc.isObject()) {
                    const QJsonObject layoutObject = layoutDoc.object();

                    const QJsonObject lastProjectSession = layoutObject.value("lastProjectSession").toObject();
                    for (auto it = lastProjectSession.begin(); it != lastProjectSession.end(); ++it) {
                        const QString directory = normalizePath(it.key());
                        if (!pathBelongsToProject(directory, normalizedProjectPath) &&
                            !pathBelongsToProject(normalizedProjectPath, directory)) {
                            continue;
                        }
                        const QString sessionId = it.value().toObject().value("id").toString();
                        if (!sessionId.isEmpty() && !seen.contains(sessionId)) {
                            seen.insert(sessionId);
                            sessionIds.append(sessionId);
                        }
                    }

                    const QJsonObject lastSession = layoutObject.value("lastSession").toObject();
                    for (auto it = lastSession.begin(); it != lastSession.end(); ++it) {
                        const QString directory = normalizePath(it.key());
                        if (!pathBelongsToProject(directory, normalizedProjectPath) &&
                            !pathBelongsToProject(normalizedProjectPath, directory)) {
                            continue;
                        }
                        const QString sessionId = it.value().toString();
                        if (!sessionId.isEmpty() && !seen.contains(sessionId)) {
                            seen.insert(sessionId);
                            sessionIds.append(sessionId);
                        }
                    }
                }
            }
        }
    }

    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir dir(homeDir + "/Library/Application Support/ai.opencode.desktop");
    if (!dir.exists()) {
        return sessionIds;
    }

    const QFileInfoList files = dir.entryInfoList(
        QStringList() << "opencode.workspace*.dat",
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time);
    for (const QFileInfo& fileInfo : files) {
        QFile file(fileInfo.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QString rawContent = QString::fromUtf8(file.readAll());
        file.close();

        if (!workspaceTerminalMentionsProject(rawContent, normalizedProjectPath)) {
            continue;
        }

        const QStringList fileSessionIds = extractUniqueOpenCodeSessionIds(rawContent);
        for (const QString& sessionId : fileSessionIds) {
            if (!seen.contains(sessionId)) {
                seen.insert(sessionId);
                sessionIds.append(sessionId);
            }
        }
    }

    return sessionIds;
}

void SessionScanner::assignClaudeSessionArtifacts(QList<SessionInfo>& sessions) const
{
    QHash<QString, QList<int>> byProject;
    for (int i = 0; i < sessions.size(); ++i) {
        if (sessions[i].toolName != "Claude") {
            continue;
        }
        byProject[normalizePath(sessions[i].projectPath)].append(i);
    }

    for (auto it = byProject.begin(); it != byProject.end(); ++it) {
        const QString projectPath = it.key();
        if (projectPath.isEmpty()) {
            continue;
        }

        const QList<GenericSessionArtifact> artifacts = collectClaudeSessionArtifacts(projectPath);
        if (artifacts.isEmpty()) {
            continue;
        }

        QList<int> sortedIndexes = it.value();
        std::sort(sortedIndexes.begin(), sortedIndexes.end(), [&](int lhs, int rhs) {
            const SessionInfo& left = sessions[lhs];
            const SessionInfo& right = sessions[rhs];
            if (left.shellPid != right.shellPid) {
                return left.shellPid < right.shellPid;
            }
            return left.processPid < right.processPid;
        });

        const int assignCount = qMin(sortedIndexes.size(), artifacts.size());
        for (int i = 0; i < assignCount; ++i) {
            SessionInfo& session = sessions[sortedIndexes[i]];
            const GenericSessionArtifact& artifact = artifacts.at(i);
            session.sessionId = artifact.sessionId;
            session.sessionPath = artifact.sessionPath;
            const SessionStatus artifactStatus = inferArtifactStatus(session.toolName,
                                                                     session.sessionId,
                                                                     session.sessionPath);
            if (artifactStatus != SessionStatus::Unknown) {
                session.status = artifactStatus;
            }
        }
    }
}

void SessionScanner::resolveSessionArtifact(const QString& projectPath, const QString& toolName,
                                            const QString& cmd, QString& sessionId,
                                            QString& sessionPath) const
{
    const QString hintedSessionId = extractSessionIdFromCommand(cmd, toolName);

    if (toolName == "Claude") {
        return;
    }

    if (toolName == "Codex") {
        sessionId = hintedSessionId;
        if (!hintedSessionId.isEmpty()) {
            const QList<CodexSessionArtifact> artifacts = collectCodexSessionArtifacts();
            for (const CodexSessionArtifact& artifact : artifacts) {
                if (artifact.sessionId == hintedSessionId) {
                    sessionPath = artifact.sessionPath;
                    break;
                }
            }
        }
        return;
    }

    if (toolName == "OpenCode") {
        findLatestOpenCodeSessionArtifact(projectPath, sessionId, sessionPath);
        return;
    }
}

QList<SessionInfo> SessionScanner::detectTerminalProcessSessions(
    const QString& terminalKeyword,
    const QString& terminalLabel)
{
    QList<SessionInfo> sessions;

    QProcess proc;
    proc.start("pgrep", QStringList() << "-lf" << terminalKeyword);
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        return sessions;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) return sessions;

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        qint64 termPid = parts[0].toLongLong();

        QStringList allDescendants = getAllChildPids(termPid, 6);
        QList<qint64> shellPids;
        for (const QString& pidStr : allDescendants) {
            qint64 pid = pidStr.toLongLong();
            const QString cmd = getProcessCommand(pid);
            if (!cmd.isEmpty() && isShellProcess(cmd)) {
                shellPids.append(pid);
            }
        }

        for (qint64 shellPid : shellPids) {
            QStringList shellDescendants = getAllChildPids(shellPid, 5);
            QMap<QString, SessionInfo> groupedSessions;
        for (const QString& pidStr : shellDescendants) {
            qint64 pid = pidStr.toLongLong();
            QString cmd = getProcessCommand(pid);
            if (cmd.isEmpty()) continue;

            QString toolName = identifyToolName(cmd);
            QString cwd = getProcessWorkingDir(pid);
            if (cwd.isEmpty()) cwd = getProcessWorkingDir(shellPid);
            if (toolName.isEmpty()) continue;

            const bool hasToolAncestor = [&]() {
                qint64 parentPid = getParentPid(pid);
                QSet<qint64> ancestorVisited;
                while (parentPid > 1 && !ancestorVisited.contains(parentPid)) {
                    ancestorVisited.insert(parentPid);
                    if (parentPid <= 1 || parentPid == shellPid) {
                        break;
                    }
                    const QString parentCmd = getProcessCommand(parentPid);
                    if (!identifyToolName(parentCmd).isEmpty()) {
                        return true;
                    }
                    parentPid = getParentPid(parentPid);
                }
                return false;
            }();
            if (hasToolAncestor) {
                continue;
            }

            quint64 ticks = getProcessCpuTicks(pid);
            const bool isRunning = isProcessRunning(pid);

                SessionInfo si;
                si.terminalName = terminalLabel;
                si.terminalIconPath = terminalIconPath(terminalLabel);
                si.terminalPid = termPid;
                si.shellPid = shellPid;
                si.processPid = pid;
                si.toolName = toolName;
                si.toolIconPath = toolIconPath(toolName);
                si.projectPath = normalizePath(cwd);
                si.lastActivity = QDateTime::currentDateTime();
                si.processStartedAt = getProcessStartedAt(pid);
                si.detailText = cmd;
                si.runtimeSeconds = 0;
                resolveSessionArtifact(si.projectPath, toolName, cmd, si.sessionId, si.sessionPath);
                if (si.sessionId.isEmpty()) {
                    si.sessionId = QString("unknown");
                }
                si.status = determineStatus(pid, ticks, isRunning, si.toolName, si.sessionId, si.sessionPath);
                const qint64 runningSeconds = si.lastActivity.isValid()
                    ? qMax<qint64>(0, si.lastActivity.secsTo(QDateTime::currentDateTime()))
                    : 0;
                if (!si.sessionPath.isEmpty()) {
                    const QFileInfo sessionFile(si.sessionPath);
                    if (sessionFile.exists() && sessionFile.lastModified().isValid()) {
                        si.lastActivity = sessionFile.lastModified();
                        si.runtimeSeconds = qMax<qint64>(0, sessionFile.lastModified().secsTo(QDateTime::currentDateTime()));
                    } else {
                        si.runtimeSeconds = runningSeconds;
                    }
                } else {
                    si.runtimeSeconds = runningSeconds;
                }

                const QString dedupeSessionId = hasKnownSessionId(si.sessionId)
                    ? si.sessionId
                    : (!si.sessionPath.isEmpty()
                        ? si.sessionPath
                        : QString("shell-%1").arg(shellPid));
                const QString groupKey = QString("%1|%2|%3")
                    .arg(si.projectPath, si.toolName, dedupeSessionId);

                if (!groupedSessions.contains(groupKey)) {
                    groupedSessions.insert(groupKey, si);
                    continue;
                }

                SessionInfo existing = groupedSessions.value(groupKey);
                if (isBetterSessionCandidate(si, existing)) {
                    groupedSessions[groupKey] = si;
                }
            }

            for (auto it = groupedSessions.begin(); it != groupedSessions.end(); ++it) {
                sessions.append(it.value());
            }
        }
    }

    return sessions;
}

QList<SessionInfo> SessionScanner::detectAiProcessSessions() const
{
    QList<SessionInfo> sessions;

    QProcess proc;
    proc.start("ps", QStringList() << "-axo" << "pid,ppid,comm,args");
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        return sessions;
    }

    const QString output = QString::fromUtf8(proc.readAllStandardOutput());
    if (output.trimmed().isEmpty()) {
        return sessions;
    }

    struct ProcessRow {
        qint64 pid = 0;
        qint64 ppid = 0;
        QString comm;
        QString args;
    };

    QHash<qint64, ProcessRow> rows;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (int i = 1; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                             Qt::SkipEmptyParts);
        if (parts.size() < 4) {
            continue;
        }

        ProcessRow row;
        row.pid = parts.at(0).toLongLong();
        row.ppid = parts.at(1).toLongLong();
        row.comm = parts.at(2);
        row.args = line.section(QRegularExpression(QStringLiteral("\\s+")), 3);
        rows.insert(row.pid, row);
    }

    QMap<QString, SessionInfo> groupedSessions;
    for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
        const ProcessRow& row = it.value();
        const QString toolName = identifyToolName(row.args);
        if (toolName.isEmpty()) {
            continue;
        }

        const bool hasToolAncestor = [&]() {
            qint64 ancestorPid = row.ppid;
            QSet<qint64> ancestorVisited;
            while (ancestorPid > 1 && !ancestorVisited.contains(ancestorPid)) {
                ancestorVisited.insert(ancestorPid);
                const QString parentCmd = getProcessCommand(ancestorPid);
                if (!identifyToolName(parentCmd).isEmpty()) {
                    return true;
                }
                ancestorPid = getParentPid(ancestorPid);
            }
            return false;
        }();
        if (hasToolAncestor) {
            continue;
        }

        qint64 shellPid = 0;
        qint64 terminalPid = 0;
        QString terminalName;
        qint64 currentPid = row.pid;
        QSet<qint64> visited;
        while (rows.contains(currentPid) && !visited.contains(currentPid)) {
            visited.insert(currentPid);
            const ProcessRow& current = rows.value(currentPid);
            if (shellPid == 0 && isShellProcess(current.args)) {
                shellPid = current.pid;
            }
            if (terminalPid == 0) {
                const QString detectedTerminal = terminalLabelFromCommand(current.args);
                if (!detectedTerminal.isEmpty()) {
                    terminalPid = current.pid;
                    terminalName = detectedTerminal;
                }
            }
            if (current.ppid <= 1) {
                break;
            }
            currentPid = current.ppid;
        }

        if (shellPid == 0 || terminalPid == 0 || terminalName.isEmpty()) {
            continue;
        }

        QString cwd = getProcessWorkingDir(row.pid);
        if (cwd.isEmpty()) {
            cwd = getProcessWorkingDir(shellPid);
        }

        const quint64 ticks = getProcessCpuTicks(row.pid);
        const bool isRunning = isProcessRunning(row.pid);

        SessionInfo si;
        si.terminalName = terminalName;
        si.terminalIconPath = terminalIconPath(terminalName);
        si.terminalPid = terminalPid;
        si.shellPid = shellPid;
        si.processPid = row.pid;
                si.toolName = toolName;
                si.toolIconPath = toolIconPath(toolName);
                si.projectPath = normalizePath(cwd);
                si.lastActivity = QDateTime::currentDateTime();
                si.processStartedAt = getProcessStartedAt(row.pid);
                si.detailText = row.args;
                si.runtimeSeconds = 0;
                resolveSessionArtifact(si.projectPath, toolName, row.args, si.sessionId, si.sessionPath);
        if (si.sessionId.isEmpty()) {
            si.sessionId = QStringLiteral("unknown");
        }
        si.status = determineStatus(row.pid, ticks, isRunning, si.toolName, si.sessionId, si.sessionPath);

        const qint64 runningSeconds = si.lastActivity.isValid()
            ? qMax<qint64>(0, si.lastActivity.secsTo(QDateTime::currentDateTime()))
            : 0;
        if (!si.sessionPath.isEmpty()) {
            const QFileInfo sessionFile(si.sessionPath);
            if (sessionFile.exists() && sessionFile.lastModified().isValid()) {
                si.lastActivity = sessionFile.lastModified();
                si.runtimeSeconds = qMax<qint64>(0, sessionFile.lastModified().secsTo(QDateTime::currentDateTime()));
            } else {
                si.runtimeSeconds = runningSeconds;
            }
        } else {
            si.runtimeSeconds = runningSeconds;
        }

        const QString dedupeSessionId = hasKnownSessionId(si.sessionId)
            ? si.sessionId
            : (!si.sessionPath.isEmpty()
                ? si.sessionPath
                : QString("shell-%1").arg(shellPid));
        const QString groupKey = QString("%1|%2|%3")
            .arg(si.projectPath, si.toolName, dedupeSessionId);

        if (!groupedSessions.contains(groupKey)) {
            groupedSessions.insert(groupKey, si);
            continue;
        }

        const SessionInfo existing = groupedSessions.value(groupKey);
        if (isBetterSessionCandidate(si, existing)) {
            groupedSessions[groupKey] = si;
        }
    }

    for (auto it = groupedSessions.cbegin(); it != groupedSessions.cend(); ++it) {
        sessions.append(it.value());
    }
    return sessions;
}

void SessionScanner::assignCodexSessionArtifacts(QList<SessionInfo>& sessions) const
{
    QList<int> codexIndexes;
    for (int i = 0; i < sessions.size(); ++i) {
        if (sessions[i].toolName == "Codex") {
            codexIndexes.append(i);
        }
    }
    if (codexIndexes.isEmpty()) {
        return;
    }

    const QList<CodexSessionArtifact> artifacts = collectCodexSessionArtifacts();
    const QList<CodexLogSessionRecord> logRecords = collectCodexSessionLogRecords();
    if (artifacts.isEmpty() && logRecords.isEmpty()) {
        return;
    }

    QHash<QString, CodexSessionArtifact> artifactById;
    for (const CodexSessionArtifact& artifact : artifacts) {
        if (!artifact.sessionId.isEmpty() && !artifactById.contains(artifact.sessionId)) {
            artifactById.insert(artifact.sessionId, artifact);
        }
    }

    auto applyArtifact = [&](SessionInfo& session, const CodexSessionArtifact& artifact) {
        session.sessionId = artifact.sessionId;
        session.sessionPath = artifact.sessionPath;
        if (!artifact.projectPath.isEmpty()) {
            session.projectPath = artifact.projectPath;
        }
        const SessionStatus artifactStatus = inferArtifactStatus(session.toolName, session.sessionId,
                                                                 session.sessionPath);
        if (artifactStatus != SessionStatus::Unknown) {
            session.status = artifactStatus;
        }
    };

    QSet<QString> usedPaths;
    QSet<QString> usedSessionIds;

    for (int index : codexIndexes) {
        SessionInfo& session = sessions[index];
        if (!hasKnownSessionId(session.sessionId)) {
            continue;
        }
        usedSessionIds.insert(session.sessionId);
        const auto artifactIt = artifactById.constFind(session.sessionId);
        if (artifactIt != artifactById.cend()) {
            applyArtifact(session, artifactIt.value());
            usedPaths.insert(artifactIt->sessionPath);
        }
    }

    QHash<QString, QList<int>> byProject;
    for (int index : codexIndexes) {
        SessionInfo& session = sessions[index];
        if (hasKnownSessionId(session.sessionId)) {
            continue;
        }
        byProject[QDir::cleanPath(session.projectPath)].append(index);
    }

    for (auto it = byProject.begin(); it != byProject.end(); ++it) {
        QList<int> sortedIndexes = it.value();
        std::sort(sortedIndexes.begin(), sortedIndexes.end(), [&](int lhs, int rhs) {
            const SessionInfo& left = sessions[lhs];
            const SessionInfo& right = sessions[rhs];
            if (left.processStartedAt.isValid() && right.processStartedAt.isValid() &&
                left.processStartedAt != right.processStartedAt) {
                return left.processStartedAt > right.processStartedAt;
            }
            return left.processPid > right.processPid;
        });

        const QString projectPath = it.key();
        QList<CodexSessionArtifact> matchedArtifacts;
        for (const CodexSessionArtifact& artifact : artifacts) {
            if (usedPaths.contains(artifact.sessionPath)) {
                continue;
            }
            if (projectPath.isEmpty()) {
                continue;
            }
            if (!pathBelongsToProject(projectPath, artifact.projectPath) &&
                !pathBelongsToProject(artifact.projectPath, projectPath)) {
                continue;
            }
            matchedArtifacts.append(artifact);
        }

        QList<int> remainingIndexes = sortedIndexes;
        QList<CodexSessionArtifact> remainingArtifacts = matchedArtifacts;
        while (!remainingIndexes.isEmpty() && !remainingArtifacts.isEmpty()) {
            int bestSessionIndex = -1;
            int bestArtifactIndex = -1;
            qint64 bestDelta = std::numeric_limits<qint64>::max();

            for (int sessionIndex : remainingIndexes) {
                const SessionInfo& session = sessions[sessionIndex];
                const QDateTime processStartedAt = session.processStartedAt;
                if (!processStartedAt.isValid()) {
                    continue;
                }

                for (int artifactIndex = 0; artifactIndex < remainingArtifacts.size(); ++artifactIndex) {
                    const CodexSessionArtifact& artifact = remainingArtifacts.at(artifactIndex);
                    if (!artifact.startedAt.isValid()) {
                        continue;
                    }
                    const qint64 delta = qAbs(artifact.startedAt.secsTo(processStartedAt));
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestSessionIndex = sessionIndex;
                        bestArtifactIndex = artifactIndex;
                    }
                }
            }

            if (bestSessionIndex < 0 || bestArtifactIndex < 0) {
                SessionInfo& session = sessions[remainingIndexes.first()];
                applyArtifact(session, remainingArtifacts.first());
                usedSessionIds.insert(remainingArtifacts.first().sessionId);
                usedPaths.insert(remainingArtifacts.first().sessionPath);
                remainingIndexes.removeFirst();
                remainingArtifacts.removeFirst();
                continue;
            }

            applyArtifact(sessions[bestSessionIndex], remainingArtifacts.at(bestArtifactIndex));
            usedSessionIds.insert(remainingArtifacts.at(bestArtifactIndex).sessionId);
            usedPaths.insert(remainingArtifacts.at(bestArtifactIndex).sessionPath);
            remainingIndexes.removeAll(bestSessionIndex);
            remainingArtifacts.removeAt(bestArtifactIndex);
        }

        const QList<CodexLogSessionRecord> projectLogs =
            collectCodexLogCandidatesForProject(logRecords, it.key(), usedSessionIds);
        QList<int> unresolvedIndexes;
        for (int index : sortedIndexes) {
            if (!hasKnownSessionId(sessions[index].sessionId)) {
                unresolvedIndexes.append(index);
            }
        }
        const int logAssignCount = qMin(unresolvedIndexes.size(), projectLogs.size());
        for (int i = 0; i < logAssignCount; ++i) {
            SessionInfo& session = sessions[unresolvedIndexes.at(i)];
            const CodexLogSessionRecord& record = projectLogs.at(i);
            session.sessionId = record.sessionId;
            if (session.projectPath.isEmpty()) {
                session.projectPath = record.projectPath;
            }
            usedSessionIds.insert(record.sessionId);

            const auto artifactIt = artifactById.constFind(record.sessionId);
            if (artifactIt != artifactById.cend() &&
                !usedPaths.contains(artifactIt->sessionPath)) {
                applyArtifact(session, artifactIt.value());
                usedPaths.insert(artifactIt->sessionPath);
            }
        }
    }
}

void SessionScanner::assignOpenCodeSessionMetadata(QList<SessionInfo>& sessions) const
{
    QHash<QString, QList<int>> byProject;
    for (int i = 0; i < sessions.size(); ++i) {
        if (sessions[i].toolName != "OpenCode") {
            continue;
        }
        byProject[normalizePath(sessions[i].projectPath)].append(i);
    }

    for (auto it = byProject.begin(); it != byProject.end(); ++it) {
        const QString projectPath = it.key();
        if (projectPath.isEmpty()) {
            continue;
        }

        const QStringList candidateIds = collectOpenCodeSessionIdsForProject(projectPath);
        if (candidateIds.isEmpty()) {
            continue;
        }

        QString inferredSessionPath;
        QString ignoredSessionId;
        findLatestOpenCodeSessionArtifact(projectPath, ignoredSessionId, inferredSessionPath);

        QList<int> sortedIndexes = it.value();
        std::sort(sortedIndexes.begin(), sortedIndexes.end(), [&](int lhs, int rhs) {
            const SessionInfo& left = sessions[lhs];
            const SessionInfo& right = sessions[rhs];
            if (left.processStartedAt.isValid() && right.processStartedAt.isValid() &&
                left.processStartedAt != right.processStartedAt) {
                return left.processStartedAt > right.processStartedAt;
            }
            return left.processPid > right.processPid;
        });

        const int assignCount = qMin(sortedIndexes.size(), candidateIds.size());
        for (int i = 0; i < assignCount; ++i) {
            SessionInfo& session = sessions[sortedIndexes.at(i)];
            session.sessionId = candidateIds.at(i);
            if (session.sessionPath.isEmpty() && !inferredSessionPath.isEmpty()) {
                session.sessionPath = inferredSessionPath;
            }
            const SessionStatus artifactStatus = inferArtifactStatus(session.toolName,
                                                                     session.sessionId,
                                                                     session.sessionPath);
            if (artifactStatus != SessionStatus::Unknown) {
                session.status = artifactStatus;
            }
        }
    }
}

QMap<QString, QList<SessionInfo>> SessionScanner::groupByProject(
    const QList<SessionInfo>& sessions,
    const QList<QPair<QString, QString>>& projects)
{
    QMap<QString, QList<SessionInfo>> result;

    for (const SessionInfo& si : sessions) {
        QString bestProjectPath;
        QString bestProjectName;
        int bestLen = -1;
        const QString normalizedSessionProjectPath = normalizePath(si.projectPath);

        for (const auto& proj : projects) {
            const QString& projName = proj.first;
            const QString normalizedProjectPath = normalizePath(proj.second);
            if (pathBelongsToProject(normalizedSessionProjectPath, normalizedProjectPath)) {
                if (normalizedProjectPath.length() > bestLen) {
                    bestLen = normalizedProjectPath.length();
                    bestProjectPath = normalizedProjectPath;
                    bestProjectName = projName;
                }
            }
        }

        QString key = bestProjectPath.isEmpty() ? normalizedSessionProjectPath : bestProjectPath;
        SessionInfo copy = si;
        copy.projectPath = normalizedSessionProjectPath;
        copy.projectName = bestProjectName;
        if (copy.projectName.isEmpty()) {
            QFileInfo fi(copy.projectPath);
            copy.projectName = fi.fileName();
        }
        result[key].append(copy);
    }

    return result;
}

QList<ProjectMonitorData> SessionScanner::scanLiveSessions(
    const QList<QPair<QString, QString>>& projects)
{
    QList<SessionInfo> allSessions;
    QSet<QString> seenIds;

    auto addUnique = [&](const QList<SessionInfo>& sessions) {
        for (const SessionInfo& si : sessions) {
            const QString normalizedProjectPath = normalizePath(si.projectPath);
            const QString stableSessionKey = hasKnownSessionId(si.sessionId)
                ? si.sessionId
                : (!si.sessionPath.isEmpty() ? si.sessionPath : QString::number(si.processPid));
            const QString uniqueKey = QString("%1|%2|%3|%4|%5")
                .arg(si.terminalPid)
                .arg(si.shellPid)
                .arg(si.toolName, normalizedProjectPath, stableSessionKey);
            if (!seenIds.contains(uniqueKey)) {
                seenIds.insert(uniqueKey);
                allSessions.append(si);
            }
        }
    };

    addUnique(detectAiProcessSessions());
    assignClaudeSessionArtifacts(allSessions);
    assignCodexSessionArtifacts(allSessions);
    assignOpenCodeSessionMetadata(allSessions);

    QList<ProjectMonitorData> result;
    QMap<QString, QList<SessionInfo>> grouped = groupByProject(allSessions, projects);

    for (const auto& proj : projects) {
        ProjectMonitorData pmd;
        pmd.projectName = proj.first;
        pmd.projectPath = proj.second;
        pmd.totalWorking = 0;
        pmd.totalReady = 0;

        const QList<SessionInfo> sessions = grouped.value(proj.second);
        QList<SessionInfo> sortedSessions = sessions;
        std::sort(sortedSessions.begin(), sortedSessions.end(), [](const SessionInfo& lhs, const SessionInfo& rhs) {
            if (lhs.toolName != rhs.toolName) {
                return lhs.toolName < rhs.toolName;
            }
            if (lhs.sessionId != rhs.sessionId) {
                return lhs.sessionId < rhs.sessionId;
            }
            return lhs.processPid < rhs.processPid;
        });

        for (const SessionInfo& si : sortedSessions) {
            pmd.sessions.append(si);
            if (si.status == SessionStatus::Working) {
                pmd.totalWorking++;
            } else if (si.status == SessionStatus::Ready) {
                pmd.totalReady++;
            }
        }

        result.append(pmd);
    }

    return result;
}
