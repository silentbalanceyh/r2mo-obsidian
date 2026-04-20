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
#include <QStandardPaths>
#include <algorithm>

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
QStringList extractUniqueOpenCodeSessionIds(const QString& rawContent)
{
    QStringList sessionIds;
    QSet<QString> seen;
    QRegularExpression re(R"(session:(ses_[^":]+):)");
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
    if (lower.contains("wezterm")) return ":/icons/ui/terminal-wezterm.svg";
    if (lower.contains("iterm")) return ":/icons/ui/terminal-iterm.svg";
    if (lower.contains("terminal")) return ":/icons/ui/terminal-macos.svg";
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
    const QByteArray tail = readArtifactTail(sessionPath);
    if (tail.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QJsonObject lastObject = parseLastJsonObject(tail);
    if (lastObject.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QString type = lastObject.value("type").toString();
    if (type == "task_complete") {
        return SessionStatus::Ready;
    }
    if (type == "task_started") {
        return SessionStatus::Working;
    }

    if (type == "event_msg") {
        const QJsonObject payload = lastObject.value("payload").toObject();
        const QString payloadType = payload.value("type").toString();
        if (payloadType == "exec_command_begin" ||
            payloadType == "function_call_begin" ||
            payloadType == "patch_apply_begin" ||
            payloadType == "agent_reasoning") {
            return SessionStatus::Working;
        }
    }

    if (type == "response_item") {
        const QJsonObject payload = lastObject.value("payload").toObject();
        const QString payloadType = payload.value("type").toString();
        if (payloadType == "function_call" ||
            payloadType == "custom_tool_call" ||
            payloadType == "reasoning") {
            return SessionStatus::Working;
        }
        if (payloadType == "message") {
            const QString phase = payload.value("phase").toString();
            if (phase == "final_answer") {
                return SessionStatus::Ready;
            }
        }
    }

    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::inferClaudeArtifactStatus(const QString& sessionPath) const
{
    const QByteArray tail = readArtifactTail(sessionPath);
    if (tail.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QJsonObject lastObject = parseLastJsonObject(tail);
    if (lastObject.isEmpty()) {
        return SessionStatus::Unknown;
    }

    const QString type = lastObject.value("type").toString();
    const QString subtype = lastObject.value("subtype").toString();
    if (subtype == "stop_hook_summary") {
        return SessionStatus::Ready;
    }

    if (type == "attachment") {
        const QJsonObject attachment = lastObject.value("attachment").toObject();
        const QString hookEvent = attachment.value("hookEvent").toString();
        const QString attachmentType = attachment.value("type").toString();
        if (hookEvent == "PreToolUse" || hookEvent == "PostToolUse") {
            return SessionStatus::Working;
        }
        if (hookEvent == "Stop" ||
            attachmentType == "hook_success" ||
            attachmentType == "async_hook_response") {
            return SessionStatus::Ready;
        }
    }

    if (type == "progress") {
        const QJsonObject data = lastObject.value("data").toObject();
        const QString hookEvent = data.value("hookEvent").toString();
        if (hookEvent == "PreToolUse" || hookEvent == "PostToolUse") {
            return SessionStatus::Working;
        }
        if (hookEvent == "Stop") {
            return SessionStatus::Ready;
        }
    }

    if (type == "assistant") {
        const QJsonObject message = lastObject.value("message").toObject();
        const QString stopReason = message.value("stop_reason").toString();
        if (stopReason == "tool_use") {
            return SessionStatus::Working;
        }
        if (stopReason == "end_turn") {
            return SessionStatus::Ready;
        }
    }

    if (type == "system") {
        if (subtype == "turn_duration" || subtype == "away_summary") {
            return SessionStatus::Ready;
        }
    }

    if (type == "user" ||
        type == "last-prompt" ||
        type == "file-history-snapshot" ||
        type == "permission-mode") {
        return SessionStatus::Ready;
    }

    if (type == "tool_use") {
        return SessionStatus::Working;
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
    if (sessionPath.isEmpty()) {
        return SessionStatus::Unknown;
    }

    if (toolName == "Codex") {
        return inferCodexArtifactStatus(sessionPath);
    }
    if (toolName == "Claude") {
        return inferClaudeArtifactStatus(sessionPath);
    }
    if (toolName == "OpenCode") {
        const SessionStatus globalStatus = inferOpenCodeGlobalStatus(sessionId);
        if (globalStatus != SessionStatus::Unknown) {
            return globalStatus;
        }
        return inferOpenCodeArtifactStatus(sessionId, sessionPath);
    }

    return SessionStatus::Unknown;
}

SessionStatus SessionScanner::determineStatus(qint64 pid, quint64 currentTicks, bool isRunning,
                                              const QString& toolName, const QString& sessionId,
                                              const QString& sessionPath)
{
    const SessionStatus artifactStatus = inferArtifactStatus(toolName, sessionId, sessionPath);
    if (artifactStatus != SessionStatus::Unknown) {
        if (artifactStatus == SessionStatus::Working) {
            s_lastWorkingTime[pid] = QDateTime::currentDateTime();
        }
        return artifactStatus;
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

    if (strongCpuBurst || (isRunning && deltaPerSecond >= 25000.0)) {
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
        if (secsSinceWork < 2) {
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
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString escaped = projectPath;
    escaped.replace("/", "-");
    QString projDir = homeDir + "/.claude/projects/" + escaped;
    QDir dir(projDir);
    if (!dir.exists()) {
        dir.setPath(homeDir + "/.claude/projects/-" + escaped);
    }

    if (!dir.exists()) {
        return;
    }

    const QFileInfoList files = dir.entryInfoList(QStringList() << "*.jsonl" << "*.json", QDir::Files, QDir::Time);
    for (const QFileInfo& fi : files) {
        const QString base = fi.completeBaseName();
        if (base.length() >= 32 || base.count("-") >= 4) {
            sessionId = base;
            sessionPath = fi.filePath();
            return;
        }
    }
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
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray head = file.read(8192);
            const QJsonObject firstObject = parseFirstJsonObject(head);
            if (firstObject.value("type").toString() == "session_meta") {
                artifactProjectPath = QDir::cleanPath(firstObject.value("payload").toObject().value("cwd").toString());
            }
        }

        CodexSessionArtifact artifact;
        artifact.sessionId = sessionIdFromRolloutBaseName(base);
        artifact.sessionPath = fi.filePath();
        artifact.projectPath = artifactProjectPath;
        artifact.lastModified = fi.lastModified();
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
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir dir(homeDir + "/Library/Application Support/ai.opencode.desktop");
    if (!dir.exists()) {
        return;
    }

    const QFileInfoList files = dir.entryInfoList(
        QStringList() << "opencode.workspace*.dat",
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time);

    QFileInfo fallbackFile;
    QString fallbackId;
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

        if (fallbackId.isEmpty()) {
            fallbackId = sessionIds.last();
            fallbackFile = fileInfo;
        }

        if (workspaceTerminalMentionsProject(rawContent, projectPath)) {
            sessionId = sessionIds.last();
            sessionPath = fileInfo.filePath();
            return;
        }
    }

    if (!fallbackId.isEmpty()) {
        sessionId = fallbackId;
        sessionPath = fallbackFile.filePath();
    }
}

void SessionScanner::resolveSessionArtifact(const QString& projectPath, const QString& toolName,
                                            const QString& cmd, QString& sessionId,
                                            QString& sessionPath) const
{
    const QString hintedSessionId = extractSessionIdFromCommand(cmd, toolName);

    if (toolName == "Claude") {
        findLatestClaudeSessionArtifact(projectPath, sessionId, sessionPath);
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

                quint64 ticks = getProcessCpuTicks(pid);
                const bool isRunning = isProcessRunning(pid);

                SessionInfo si;
                si.terminalName = terminalLabel;
                si.terminalIconPath = terminalIconPath(terminalLabel);
                si.terminalPid = termPid;
                si.processPid = pid;
                si.toolName = toolName;
                si.toolIconPath = toolIconPath(toolName);
                si.projectPath = cwd;
                si.lastActivity = QDateTime::currentDateTime();
                si.detailText = cmd;
                resolveSessionArtifact(cwd, toolName, cmd, si.sessionId, si.sessionPath);
                if (si.sessionId.isEmpty()) {
                    si.sessionId = QString("unknown");
                }
                si.status = determineStatus(pid, ticks, isRunning, si.toolName, si.sessionId, si.sessionPath);

                const QString dedupeSessionId = (si.sessionId.isEmpty() || si.sessionId == "unknown")
                    ? QString("pid-%1").arg(pid)
                    : si.sessionId;
                const QString groupKey = QString("%1|%2|%3")
                    .arg(si.projectPath, si.toolName, dedupeSessionId);

                if (!groupedSessions.contains(groupKey)) {
                    groupedSessions.insert(groupKey, si);
                    continue;
                }

                SessionInfo existing = groupedSessions.value(groupKey);
                if (si.status == SessionStatus::Working && existing.status != SessionStatus::Working) {
                    groupedSessions[groupKey] = si;
                    continue;
                }

                if (si.status == existing.status && isRunning && !isProcessRunning(existing.processPid)) {
                    groupedSessions[groupKey] = si;
                    continue;
                }

                if (si.status == existing.status && si.processPid > existing.processPid) {
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
    if (artifacts.isEmpty()) {
        return;
    }

    QSet<QString> usedPaths;

    for (int index : codexIndexes) {
        SessionInfo& session = sessions[index];
        if (session.sessionId.isEmpty() || session.sessionId == "unknown") {
            continue;
        }
        for (const CodexSessionArtifact& artifact : artifacts) {
            if (artifact.sessionId == session.sessionId) {
                session.sessionPath = artifact.sessionPath;
                usedPaths.insert(artifact.sessionPath);
                break;
            }
        }
    }

    QHash<QString, QList<int>> byProject;
    for (int index : codexIndexes) {
        SessionInfo& session = sessions[index];
        if (!session.sessionPath.isEmpty()) {
            continue;
        }
        byProject[QDir::cleanPath(session.projectPath)].append(index);
    }

    for (auto it = byProject.begin(); it != byProject.end(); ++it) {
        const QString projectPath = it.key();
        QList<CodexSessionArtifact> matched;
        QList<CodexSessionArtifact> fallback;

        for (const CodexSessionArtifact& artifact : artifacts) {
            if (usedPaths.contains(artifact.sessionPath)) {
                continue;
            }
            if (!projectPath.isEmpty() && artifact.projectPath == projectPath) {
                matched.append(artifact);
            } else if (artifact.projectPath.isEmpty()) {
                fallback.append(artifact);
            }
        }

        QList<CodexSessionArtifact> pool = matched;
        pool.append(fallback);
        const QList<int>& indexes = it.value();
        const int assignCount = qMin(indexes.size(), pool.size());
        for (int i = 0; i < assignCount; ++i) {
            SessionInfo& session = sessions[indexes[i]];
            session.sessionId = pool[i].sessionId;
            session.sessionPath = pool[i].sessionPath;
            usedPaths.insert(pool[i].sessionPath);
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

        for (const auto& proj : projects) {
            const QString& projName = proj.first;
            const QString& projPath = proj.second;
            if (si.projectPath == projPath || si.projectPath.startsWith(projPath + "/")) {
                if (projPath.length() > bestLen) {
                    bestLen = projPath.length();
                    bestProjectPath = projPath;
                    bestProjectName = projName;
                }
            }
        }

        QString key = bestProjectPath.isEmpty() ? si.projectPath : bestProjectPath;
        SessionInfo copy = si;
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
            const QString uniqueKey = QString("%1|%2|%3|%4")
                .arg(si.terminalPid)
                .arg(si.processPid)
                .arg(si.projectPath, si.toolName);
            if (!seenIds.contains(uniqueKey)) {
                seenIds.insert(uniqueKey);
                allSessions.append(si);
            }
        }
    };

    addUnique(detectTerminalProcessSessions("wezterm", "WezTerm"));
    addUnique(detectTerminalProcessSessions("iTerm", "iTerm2"));
    addUnique(detectTerminalProcessSessions("Terminal", "Terminal.app"));
    assignCodexSessionArtifacts(allSessions);

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
