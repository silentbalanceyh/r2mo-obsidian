#ifndef REMOTESESSIONSCANNER_H
#define REMOTESESSIONSCANNER_H

#include <QList>

#include "models/vaultmodel.h"
#include "sessionscanner.h"
#include "sshremoteexecutor.h"

class RemoteSessionScanner
{
public:
    RemoteSessionScanner();

    QList<ProjectMonitorData> scanRemoteVault(const Vault& vault) const;

private:
    SshRemoteExecutor m_executor;
};

#endif // REMOTESESSIONSCANNER_H
