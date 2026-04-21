#include "vaultmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QSet>
#include <QDir>

namespace {
QString vaultKindToString(VaultKind kind)
{
    switch (kind) {
    case VaultKind::Remote:
        return QStringLiteral("remote");
    case VaultKind::Local:
    default:
        return QStringLiteral("local");
    }
}

VaultKind vaultKindFromString(const QString& value)
{
    return value.compare(QStringLiteral("remote"), Qt::CaseInsensitive) == 0
        ? VaultKind::Remote
        : VaultKind::Local;
}

QString connectionStatusToString(VaultConnectionStatus status)
{
    switch (status) {
    case VaultConnectionStatus::Connected:
        return QStringLiteral("connected");
    case VaultConnectionStatus::Disconnected:
        return QStringLiteral("disconnected");
    case VaultConnectionStatus::Unknown:
    default:
        return QStringLiteral("unknown");
    }
}

VaultConnectionStatus connectionStatusFromString(const QString& value)
{
    if (value.compare(QStringLiteral("connected"), Qt::CaseInsensitive) == 0) {
        return VaultConnectionStatus::Connected;
    }
    if (value.compare(QStringLiteral("disconnected"), Qt::CaseInsensitive) == 0) {
        return VaultConnectionStatus::Disconnected;
    }
    return VaultConnectionStatus::Unknown;
}
}

QJsonObject Vault::toJson() const
{
    QJsonObject obj;
    obj["kind"] = vaultKindToString(kind);
    obj["name"] = name;
    obj["path"] = path;
    obj["addedAt"] = addedAt.toString(Qt::ISODate);
    obj["host"] = host;
    obj["username"] = username;
    obj["password"] = password;
    obj["useKeyAuth"] = useKeyAuth;
    obj["remotePath"] = remotePath;
    obj["connectionStatus"] = connectionStatusToString(connectionStatus);
    obj["lastConnectionCheck"] = lastConnectionCheck.toString(Qt::ISODate);
    obj["lastConnectionError"] = lastConnectionError;
    return obj;
}

Vault Vault::fromJson(const QJsonObject& json)
{
    Vault v;
    v.kind = vaultKindFromString(json["kind"].toString());
    v.name = json["name"].toString();
    v.path = json["path"].toString();
    v.addedAt = QDateTime::fromString(json["addedAt"].toString(), Qt::ISODate);
    v.host = json["host"].toString();
    v.username = json["username"].toString();
    v.password = json["password"].toString();
    v.useKeyAuth = json.contains("useKeyAuth") ? json["useKeyAuth"].toBool(true) : true;
    v.remotePath = json["remotePath"].toString();
    v.connectionStatus = connectionStatusFromString(json["connectionStatus"].toString());
    v.lastConnectionCheck = QDateTime::fromString(json["lastConnectionCheck"].toString(), Qt::ISODate);
    v.lastConnectionError = json["lastConnectionError"].toString();
    if (v.kind == VaultKind::Remote && v.path.trimmed().isEmpty()) {
        v.path = buildRemoteIdentifier(v.username, v.host, v.remotePath);
    }
    return v;
}

bool Vault::isRemote() const
{
    return kind == VaultKind::Remote;
}

QString Vault::connectionStatusText() const
{
    switch (connectionStatus) {
    case VaultConnectionStatus::Connected:
        return QStringLiteral("connected");
    case VaultConnectionStatus::Disconnected:
        return QStringLiteral("disconnected");
    case VaultConnectionStatus::Unknown:
    default:
        return QStringLiteral("unknown");
    }
}

QString Vault::buildRemoteIdentifier(const QString& username, const QString& host, const QString& remotePath)
{
    const QString normalizedRemotePath = QDir::cleanPath(remotePath.trimmed());
    return QStringLiteral("ssh://%1@%2%3")
        .arg(username.trimmed(), host.trimmed(), normalizedRemotePath.startsWith('/') ? normalizedRemotePath : QStringLiteral("/") + normalizedRemotePath);
}

VaultModel::VaultModel(QObject* parent)
    : QObject(parent)
{
}

void VaultModel::addVault(const Vault& vault)
{
    if (!contains(vault.path)) {
        m_vaults.append(vault);
        emit vaultAdded(vault);
        emit modelChanged();
    }
}

void VaultModel::removeVault(const QString& path)
{
    int idx = indexOf(path);
    if (idx >= 0) {
        m_vaults.removeAt(idx);
        emit vaultRemoved(path);
        emit modelChanged();
    }
}

void VaultModel::updateVault(const QString& path, const Vault& vault)
{
    int idx = indexOf(path);
    if (idx >= 0) {
        m_vaults[idx] = vault;
        emit vaultUpdated(vault);
        emit modelChanged();
    }
}

void VaultModel::reorderVaults(const QStringList& orderedPaths)
{
    if (orderedPaths.isEmpty() || orderedPaths.size() != m_vaults.size()) {
        return;
    }

    QList<Vault> reordered;
    reordered.reserve(m_vaults.size());
    QSet<QString> seenPaths;

    for (const QString& path : orderedPaths) {
        const int index = indexOf(path);
        if (index < 0 || seenPaths.contains(path)) {
            return;
        }
        reordered.append(m_vaults.at(index));
        seenPaths.insert(path);
    }

    bool changed = false;
    for (int i = 0; i < m_vaults.size(); ++i) {
        if (m_vaults.at(i).path != reordered.at(i).path) {
            changed = true;
            break;
        }
    }

    if (!changed) {
        return;
    }

    m_vaults = reordered;
    emit modelChanged();
}

QList<Vault> VaultModel::vaults() const
{
    return m_vaults;
}

Vault VaultModel::vaultAt(int index) const
{
    if (index >= 0 && index < m_vaults.size()) {
        return m_vaults[index];
    }
    return Vault();
}

int VaultModel::count() const
{
    return m_vaults.size();
}

bool VaultModel::isEmpty() const
{
    return m_vaults.isEmpty();
}

bool VaultModel::contains(const QString& path) const
{
    return indexOf(path) >= 0;
}

int VaultModel::indexOf(const QString& path) const
{
    for (int i = 0; i < m_vaults.size(); ++i) {
        if (m_vaults[i].path == path) {
            return i;
        }
    }
    return -1;
}

void VaultModel::load(const QString& configPath)
{
    QFile file(configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray array = doc.array();
        
        m_vaults.clear();
        for (const QJsonValue& val : array) {
            m_vaults.append(Vault::fromJson(val.toObject()));
        }
        emit modelChanged();
    }
}

void VaultModel::save(const QString& configPath)
{
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray array;
        for (const Vault& v : m_vaults) {
            array.append(v.toJson());
        }
        QJsonDocument doc(array);
        file.write(doc.toJson());
    }
}
