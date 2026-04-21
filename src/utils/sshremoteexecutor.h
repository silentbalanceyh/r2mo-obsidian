#ifndef SSHREMOTEEXECUTOR_H
#define SSHREMOTEEXECUTOR_H

#include <QString>
#include <QStringList>

struct SshRemoteCommandResult {
    bool success = false;
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
};

class SshRemoteExecutor
{
public:
    SshRemoteExecutor();

    SshRemoteCommandResult runCommand(const QString& host,
                                      const QString& username,
                                      const QString& password,
                                      bool useKeyAuth,
                                      const QString& remoteCommand,
                                      int timeoutMs = 15000) const;

    SshRemoteCommandResult listDirectories(const QString& host,
                                           const QString& username,
                                           const QString& password,
                                           bool useKeyAuth,
                                           const QString& remotePath,
                                           int timeoutMs = 15000) const;

private:
    QString buildTarget(const QString& host, const QString& username) const;
    QString quoteShell(const QString& value) const;
    QString buildExpectScript(const QString& target,
                              const QString& password,
                              const QString& remoteCommand) const;
};

#endif // SSHREMOTEEXECUTOR_H
