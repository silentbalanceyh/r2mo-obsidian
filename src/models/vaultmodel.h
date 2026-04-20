#ifndef VAULTMODEL_H
#define VAULTMODEL_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDateTime>

struct Vault
{
    QString name;
    QString path;
    QDateTime addedAt;
    
    QJsonObject toJson() const;
    static Vault fromJson(const QJsonObject& json);
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
