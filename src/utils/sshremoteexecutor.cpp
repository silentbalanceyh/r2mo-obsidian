#include "sshremoteexecutor.h"

#include <QProcess>

SshRemoteExecutor::SshRemoteExecutor() = default;

QString SshRemoteExecutor::buildTarget(const QString& host, const QString& username) const
{
    return QStringLiteral("%1@%2").arg(username.trimmed(), host.trimmed());
}

QString SshRemoteExecutor::quoteShell(const QString& value) const
{
    QString escaped = value;
    escaped.replace('\'', QStringLiteral("'\"'\"'"));
    return QStringLiteral("'%1'").arg(escaped);
}

QString SshRemoteExecutor::buildExpectScript(const QString& target,
                                             const QString& password,
                                             const QString& remoteCommand) const
{
    QString escapedPassword = password;
    escapedPassword.replace("\\", "\\\\");
    escapedPassword.replace("\"", "\\\"");
    escapedPassword.replace("$", "\\$");
    escapedPassword.replace("[", "\\[");
    escapedPassword.replace("]", "\\]");

    return QStringLiteral(
        "set timeout 15\n"
        "set password \"%1\"\n"
        "spawn ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=10 %2 %3\n"
        "expect {\n"
        "    -re \".*yes/no.*\" { send \"yes\\r\"; exp_continue }\n"
        "    -re \".*[Pp]assword:.*\" { send \"$password\\r\"; exp_continue }\n"
        "    eof\n"
        "}\n"
        "catch wait result\n"
        "set exit_code [lindex $result 3]\n"
        "exit $exit_code\n")
        .arg(escapedPassword, quoteShell(target), quoteShell(remoteCommand));
}

SshRemoteCommandResult SshRemoteExecutor::runCommand(const QString& host,
                                                     const QString& username,
                                                     const QString& password,
                                                     bool useKeyAuth,
                                                     const QString& remoteCommand,
                                                     int timeoutMs) const
{
    SshRemoteCommandResult result;
    if (host.trimmed().isEmpty() || username.trimmed().isEmpty() || remoteCommand.trimmed().isEmpty()) {
        result.standardError = QStringLiteral("Missing SSH connection parameters.");
        return result;
    }

    QProcess process;
    if (useKeyAuth) {
        const QString target = buildTarget(host, username);
        process.start(QStringLiteral("ssh"), {
            QStringLiteral("-o"), QStringLiteral("StrictHostKeyChecking=no"),
            QStringLiteral("-o"), QStringLiteral("UserKnownHostsFile=/dev/null"),
            QStringLiteral("-o"), QStringLiteral("ConnectTimeout=10"),
            target,
            remoteCommand
        });
    } else {
        const QString target = buildTarget(host, username);
        process.start(QStringLiteral("expect"), {
            QStringLiteral("-c"),
            buildExpectScript(target, password, remoteCommand)
        });
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        result.standardError = QStringLiteral("SSH command timeout.");
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    result.standardError = QString::fromUtf8(process.readAllStandardError()).trimmed();
    result.success = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    return result;
}

SshRemoteCommandResult SshRemoteExecutor::listDirectories(const QString& host,
                                                          const QString& username,
                                                          const QString& password,
                                                          bool useKeyAuth,
                                                          const QString& remotePath,
                                                          int timeoutMs) const
{
    const QString normalizedPath = remotePath.trimmed().isEmpty() ? QStringLiteral(".") : remotePath.trimmed();
    const QString command = QStringLiteral("find %1 -mindepth 1 -maxdepth 1 -type d | sort")
        .arg(quoteShell(normalizedPath));
    return runCommand(host, username, password, useKeyAuth, command, timeoutMs);
}
