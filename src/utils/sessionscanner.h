#ifndef SESSIONSCANNER_H
#define SESSIONSCANNER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>
#include <QMap>

enum class SessionStatus {
    Working,
    Ready,
    Unknown
};

struct SessionInfo {
    QString sessionId;
    QString toolName;
    QString projectName;
    QString projectPath;
    QString terminalName;
    QString terminalPid;
    SessionStatus status;
    QDateTime lastActivity;
    QString sessionPath;
    QString detailText;
};

struct ToolSessionSummary {
    QString toolName;
    int workingCount;
    int readyCount;
    int totalCount;
    QList<SessionInfo> sessions;
};

struct ProjectMonitorData {
    QString projectName;
    QString projectPath;
    QList<ToolSessionSummary> toolSummaries;
    int totalWorking;
    int totalReady;
};

class SessionScanner
{
public:
    SessionScanner();

    QList<ProjectMonitorData> scanAllProjects(const QList<QPair<QString, QString>>& projects);
    QList<SessionInfo> scanTerminalSessions();

private:
    QList<SessionInfo> detectWezTermSessions();
    QList<SessionInfo> detectiTerm2Sessions();
    QList<SessionInfo> detectTerminalSessions();

    QStringList getChildPids(qint64 parentPid) const;
    QString getProcessCommand(qint64 pid) const;
    QString getProcessWorkingDir(qint64 pid) const;
    SessionStatus inferSessionStatus(const SessionInfo& info) const;

    QMap<QString, QList<SessionInfo>> groupByProject(const QList<SessionInfo>& sessions,
                                                      const QList<QPair<QString, QString>>& projects);
};

#endif // SESSIONSCANNER_H
