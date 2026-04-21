#ifndef SESSIONSCANNER_H
#define SESSIONSCANNER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>
#include <QMap>
#include <QHash>

enum class SessionStatus {
    Working,
    Ready,
    Unknown
};

struct SessionInfo {
    QString sessionId;
    QString toolName;
    QString toolIconPath;
    QString projectName;
    QString projectPath;
    QString terminalName;
    QString terminalIconPath;
    qint64 terminalPid;
    qint64 shellPid;
    qint64 processPid;
    SessionStatus status;
    QDateTime lastActivity;
    QDateTime processStartedAt;
    qint64 runtimeSeconds;
    QString sessionPath;
    QString detailText;
};

struct ProjectMonitorData {
    QString projectName;
    QString projectPath;
    QList<SessionInfo> sessions;
    int totalWorking;
    int totalReady;
};

class SessionScanner
{
public:
    SessionScanner();

    QList<ProjectMonitorData> scanLiveSessions(const QList<QPair<QString, QString>>& projects);

    static QString toolIconPath(const QString& toolName);
    static QString terminalIconPath(const QString& terminalName);

private:
    struct GenericSessionArtifact {
        QString sessionId;
        QString sessionPath;
        QString projectPath;
        QDateTime lastModified;
    };

    struct CodexSessionArtifact {
        QString sessionId;
        QString sessionPath;
        QString projectPath;
        QDateTime lastModified;
        QDateTime startedAt;
    };

    QList<SessionInfo> detectAiProcessSessions() const;
    QList<SessionInfo> detectTerminalProcessSessions(const QString& terminalKeyword,
                                                      const QString& terminalLabel);
    QStringList getAllChildPids(qint64 parentPid, int maxDepth = 5) const;
    QStringList getChildPids(qint64 parentPid) const;
    bool isShellProcess(const QString& cmd) const;
    QString getProcessCommand(qint64 pid) const;
    QString getProcessWorkingDir(qint64 pid) const;
    QDateTime getProcessStartedAt(qint64 pid) const;
    bool isProcessRunning(qint64 pid) const;
    quint64 getProcessCpuTicks(qint64 pid) const;
    SessionStatus determineStatus(qint64 pid, quint64 currentTicks, bool isRunning,
                                  const QString& toolName, const QString& sessionId,
                                  const QString& sessionPath) const;
    QString identifyToolName(const QString& cmd) const;
    QByteArray readArtifactTail(const QString& sessionPath, qint64 maxBytes = 131072) const;
    SessionStatus inferArtifactStatus(const QString& toolName, const QString& sessionId,
                                      const QString& sessionPath) const;
    SessionStatus inferCodexArtifactStatus(const QString& sessionPath) const;
    SessionStatus inferClaudeArtifactStatus(const QString& sessionPath) const;
    SessionStatus inferOpenCodeArtifactStatus(const QString& sessionId,
                                              const QString& sessionPath) const;
    SessionStatus inferOpenCodeGlobalStatus(const QString& sessionId) const;
    QString extractSessionIdFromCommand(const QString& cmd, const QString& toolName) const;
    QList<CodexSessionArtifact> collectCodexSessionArtifacts() const;
    void assignCodexSessionArtifacts(QList<SessionInfo>& sessions) const;
    void assignClaudeSessionArtifacts(QList<SessionInfo>& sessions) const;
    void assignOpenCodeSessionMetadata(QList<SessionInfo>& sessions) const;
    void resolveSessionArtifact(const QString& projectPath, const QString& toolName,
                                const QString& cmd, QString& sessionId, QString& sessionPath) const;
    QList<GenericSessionArtifact> collectClaudeSessionArtifacts(const QString& projectPath) const;
    void findLatestClaudeSessionArtifact(const QString& projectPath, QString& sessionId,
                                         QString& sessionPath) const;
    void findLatestOpenCodeSessionArtifact(const QString& projectPath, QString& sessionId,
                                           QString& sessionPath) const;
    QStringList collectOpenCodeSessionIdsForProject(const QString& projectPath) const;

    QMap<QString, QList<SessionInfo>> groupByProject(const QList<SessionInfo>& sessions,
                                                      const QList<QPair<QString, QString>>& projects);

    static QHash<qint64, quint64> s_prevCpuTicks;
    static QHash<qint64, qint64> s_prevSampleMs;
    static QHash<qint64, QDateTime> s_lastWorkingTime;
    static QHash<qint64, int> s_highActivitySamples;
};

#endif // SESSIONSCANNER_H
