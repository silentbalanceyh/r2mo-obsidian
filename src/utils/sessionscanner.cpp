#include "sessionscanner.h"
#include "aitoolscanner.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifdef Q_OS_MAC
#include <libproc.h>
#include <sys/sysctl.h>
#endif

SessionScanner::SessionScanner()
{
}

QStringList SessionScanner::getChildPids(qint64 parentPid) const
{
    QStringList result;
#ifdef Q_OS_MAC
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size = 0;
    if (sysctl(mib, 4, nullptr, &size, nullptr, 0) < 0) {
        return result;
    }

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

QList<SessionInfo> SessionScanner::detectWezTermSessions()
{
    QList<SessionInfo> sessions;

    QProcess proc;
    proc.start("pgrep", QStringList() << "-l" << "wezterm");
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

        qint64 pid = parts[0].toLongLong();
        QString cmd = getProcessCommand(pid);
        if (cmd.isEmpty()) continue;

        QString cwd = getProcessWorkingDir(pid);

        QStringList childPids = getChildPids(pid);
        for (const QString& childPidStr : childPids) {
            qint64 childPid = childPidStr.toLongLong();
            QString childCmd = getProcessCommand(childPid);
            if (childCmd.isEmpty()) continue;

            QString lowerCmd = childCmd.toLower();
            bool isAiTool = lowerCmd.contains("claude") || lowerCmd.contains("codex") ||
                           lowerCmd.contains("opencode") || lowerCmd.contains("gemini") ||
                           lowerCmd.contains("aider") || lowerCmd.contains("cursor");

            if (!isAiTool) {
                QString childCwd = getProcessWorkingDir(childPid);
                if (!childCwd.isEmpty()) {
                    QDir dir(childCwd);
                    if (dir.exists(".claude") || dir.exists(".codex") ||
                        dir.exists(".opencode") || dir.exists(".gemini") ||
                        dir.exists(".cursor") || dir.exists(".trae")) {
                        isAiTool = true;
                    }
                }
            }

            if (isAiTool) {
                SessionInfo si;
                si.terminalName = "WezTerm";
                si.terminalPid = QString::number(pid);
                si.toolName = QFileInfo(childCmd).fileName();
                si.projectPath = getProcessWorkingDir(childPid);
                si.lastActivity = QDateTime::currentDateTime();
                si.status = SessionStatus::Working;
                si.detailText = childCmd;
                sessions.append(si);
            }
        }
    }

    return sessions;
}

QList<SessionInfo> SessionScanner::detectiTerm2Sessions()
{
    QList<SessionInfo> sessions;

    QProcess proc;
    proc.start("pgrep", QStringList() << "-l" << "iTerm");
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

        qint64 pid = parts[0].toLongLong();

        QStringList childPids = getChildPids(pid);
        for (const QString& childPidStr : childPids) {
            qint64 childPid = childPidStr.toLongLong();
            QString childCmd = getProcessCommand(childPid);
            if (childCmd.isEmpty()) continue;

            QString lowerCmd = childCmd.toLower();
            bool isAiTool = lowerCmd.contains("claude") || lowerCmd.contains("codex") ||
                           lowerCmd.contains("opencode") || lowerCmd.contains("gemini") ||
                           lowerCmd.contains("aider") || lowerCmd.contains("cursor");

            if (!isAiTool) {
                QString childCwd = getProcessWorkingDir(childPid);
                if (!childCwd.isEmpty()) {
                    QDir dir(childCwd);
                    if (dir.exists(".claude") || dir.exists(".codex") ||
                        dir.exists(".opencode") || dir.exists(".gemini") ||
                        dir.exists(".cursor") || dir.exists(".trae")) {
                        isAiTool = true;
                    }
                }
            }

            if (isAiTool) {
                SessionInfo si;
                si.terminalName = "iTerm2";
                si.terminalPid = QString::number(pid);
                si.toolName = QFileInfo(childCmd).fileName();
                si.projectPath = getProcessWorkingDir(childPid);
                si.lastActivity = QDateTime::currentDateTime();
                si.status = SessionStatus::Working;
                si.detailText = childCmd;
                sessions.append(si);
            }
        }
    }

    return sessions;
}

QList<SessionInfo> SessionScanner::detectTerminalSessions()
{
    QList<SessionInfo> sessions;

    QProcess proc;
    proc.start("pgrep", QStringList() << "-l" << "Terminal");
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

        qint64 pid = parts[0].toLongLong();

        QStringList childPids = getChildPids(pid);
        for (const QString& childPidStr : childPids) {
            qint64 childPid = childPidStr.toLongLong();
            QString childCmd = getProcessCommand(childPid);
            if (childCmd.isEmpty()) continue;

            QString lowerCmd = childCmd.toLower();
            bool isAiTool = lowerCmd.contains("claude") || lowerCmd.contains("codex") ||
                           lowerCmd.contains("opencode") || lowerCmd.contains("gemini") ||
                           lowerCmd.contains("aider") || lowerCmd.contains("cursor");

            if (isAiTool) {
                SessionInfo si;
                si.terminalName = "Terminal";
                si.terminalPid = QString::number(pid);
                si.toolName = QFileInfo(childCmd).fileName();
                si.projectPath = getProcessWorkingDir(childPid);
                si.lastActivity = QDateTime::currentDateTime();
                si.status = SessionStatus::Working;
                si.detailText = childCmd;
                sessions.append(si);
            }
        }
    }

    return sessions;
}

SessionStatus SessionScanner::inferSessionStatus(const SessionInfo& info) const
{
    Q_UNUSED(info);
    return SessionStatus::Unknown;
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

QList<SessionInfo> SessionScanner::scanTerminalSessions()
{
    QList<SessionInfo> allSessions;
    QSet<QString> seenPids;

    auto addUnique = [&](const QList<SessionInfo>& sessions) {
        for (const SessionInfo& si : sessions) {
            QString key = si.terminalPid + ":" + si.toolName + ":" + si.projectPath;
            if (!seenPids.contains(key)) {
                seenPids.insert(key);
                allSessions.append(si);
            }
        }
    };

    addUnique(detectWezTermSessions());
    addUnique(detectiTerm2Sessions());
    addUnique(detectTerminalSessions());

    return allSessions;
}

QList<ProjectMonitorData> SessionScanner::scanAllProjects(
    const QList<QPair<QString, QString>>& projects)
{
    QList<ProjectMonitorData> result;

    QList<SessionInfo> termSessions = scanTerminalSessions();

    AIToolScanner aiScanner;
    QList<SessionInfo> fileSessions;

    for (const auto& proj : projects) {
        const QString& projName = proj.first;
        const QString& projPath = proj.second;

        QList<AIToolInfo> tools = aiScanner.scanProject(projPath);
        for (const AIToolInfo& tool : tools) {
            if (tool.sessions.count > 0) {
                for (const AIToolItem& item : tool.sessions.items) {
                    SessionInfo si;
                    si.projectName = projName;
                    si.projectPath = projPath;
                    si.toolName = tool.name;
                    si.lastActivity = item.modifiedTime;
                    si.sessionPath = item.path;
                    si.detailText = item.displayText;

                    qint64 ageSecs = item.modifiedTime.secsTo(QDateTime::currentDateTime());
                    si.status = (ageSecs < 300) ? SessionStatus::Working : SessionStatus::Ready;

                    fileSessions.append(si);
                }
            }
        }
    }

    QList<SessionInfo> allSessions;
    QSet<QString> seenSessions;

    for (const SessionInfo& si : termSessions) {
        QString key = si.projectPath + ":" + si.toolName;
        if (!seenSessions.contains(key)) {
            seenSessions.insert(key);
            allSessions.append(si);
        }
    }

    for (const SessionInfo& si : fileSessions) {
        QString key = si.projectPath + ":" + si.toolName;
        if (!seenSessions.contains(key)) {
            seenSessions.insert(key);
            allSessions.append(si);
        }
    }

    QMap<QString, QList<SessionInfo>> grouped = groupByProject(allSessions, projects);

    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        ProjectMonitorData pmd;
        pmd.projectPath = it.key();
        pmd.totalWorking = 0;
        pmd.totalReady = 0;

        QMap<QString, ToolSessionSummary> toolMap;
        for (const SessionInfo& si : it.value()) {
            if (!toolMap.contains(si.toolName)) {
                ToolSessionSummary ts;
                ts.toolName = si.toolName;
                ts.workingCount = 0;
                ts.readyCount = 0;
                ts.totalCount = 0;
                toolMap[si.toolName] = ts;
            }
            toolMap[si.toolName].sessions.append(si);
            toolMap[si.toolName].totalCount++;
            if (si.status == SessionStatus::Working) {
                toolMap[si.toolName].workingCount++;
                pmd.totalWorking++;
            } else if (si.status == SessionStatus::Ready) {
                toolMap[si.toolName].readyCount++;
                pmd.totalReady++;
            }
        }

        if (!it.value().isEmpty()) {
            pmd.projectName = it.value().first().projectName;
        } else {
            QFileInfo fi(pmd.projectPath);
            pmd.projectName = fi.fileName();
        }

        pmd.toolSummaries = toolMap.values();
        result.append(pmd);
    }

    return result;
}
