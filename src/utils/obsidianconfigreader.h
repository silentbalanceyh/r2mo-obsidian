#ifndef OBSIDIANCONFIGREADER_H
#define OBSIDIANCONFIGREADER_H

#include <QString>
#include <QList>
#include <QJsonObject>

struct ObsidianVaultInfo {
    QString id;          // Vault ID in Obsidian config
    QString path;        // Vault path
    QString name;        // Vault name (from .obsidian/app.json or directory name)
    qint64 timestamp;    // Last accessed timestamp
    bool isOpen;         // Currently open in Obsidian
    bool exists;         // Path exists on disk
    bool isValid;        // Contains .obsidian directory
};

class ObsidianConfigReader
{
public:
    static ObsidianConfigReader& instance();
    
    // Get all vaults from Obsidian config
    QList<ObsidianVaultInfo> getObsidianVaults() const;
    
    // Get vault info by path
    ObsidianVaultInfo getVaultByPath(const QString& path) const;
    
    // Get the path to Obsidian config file
    QString getConfigPath() const;
    
    // Check if Obsidian config exists
    bool hasObsidianConfig() const;
    
    // Register vault to Obsidian config and return vault ID
    QString registerVault(const QString& path);
    
    // Generate 16-character hex vault ID
    QString generateVaultId();
    
    // Check if Obsidian process is running
    bool isObsidianRunning() const;

private:
    ObsidianConfigReader() = default;
    ~ObsidianConfigReader() = default;
    
    Q_DISABLE_COPY(ObsidianConfigReader)
    
    QString getVaultName(const QString& path) const;
};

#endif // OBSIDIANCONFIGREADER_H