#include "vaultmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QSet>

QJsonObject Vault::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["path"] = path;
    obj["addedAt"] = addedAt.toString(Qt::ISODate);
    return obj;
}

Vault Vault::fromJson(const QJsonObject& json)
{
    Vault v;
    v.name = json["name"].toString();
    v.path = json["path"].toString();
    v.addedAt = QDateTime::fromString(json["addedAt"].toString(), Qt::ISODate);
    return v;
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
