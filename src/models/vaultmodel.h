#ifndef VAULTMODEL_H
#define VAULTMODEL_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDateTime>

enum class VaultKind {
    Local,
    Remote
};

enum class VaultConnectionStatus {
    Unknown,
    Connected,
    Disconnected
};

struct Vault
{
    VaultKind kind = VaultKind::Local;
    QString name;
    QString path;
    QDateTime addedAt;
    QString host;
    QString username;
    QString password;
    bool useKeyAuth = true;
    QString remotePath;
    VaultConnectionStatus connectionStatus = VaultConnectionStatus::Unknown;
    QDateTime lastConnectionCheck;
    QString lastConnectionError;
    
    QJsonObject toJson() const;
    static Vault fromJson(const QJsonObject& json);
    bool isRemote() const;
    QString connectionStatusText() const;
    static QString buildRemoteIdentifier(const QString& username, const QString& host, const QString& remotePath);
};

class VaultModel : public QObject
{
    Q_OBJECT

public:
    explicit VaultModel(QObject* parent = nullptr);
    
    void addVault(const Vault& vault);
    void removeVault(const QString& path);
    void updateVault(const QString& path, const Vault& vault);
    void reorderVaults(const QStringList& orderedPaths);
    
    QList<Vault> vaults() const;
    Vault vaultAt(int index) const;
    int count() const;
    bool isEmpty() const;
    
    bool contains(const QString& path) const;
    int indexOf(const QString& path) const;
    
    void load(const QString& configPath);
    void save(const QString& configPath);

signals:
    void vaultAdded(const Vault& vault);
    void vaultRemoved(const QString& path);
    void vaultUpdated(const Vault& vault);
    void modelChanged();

private:
    QList<Vault> m_vaults;
};

#endif // VAULTMODEL_H
