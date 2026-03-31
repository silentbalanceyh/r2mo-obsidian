#include "obsidianconfigreader.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDateTime>
#include <QProcess>
#include <cstdlib>
#include <ctime>

ObsidianConfigReader& ObsidianConfigReader::instance()
{
    static ObsidianConfigReader reader;
    return reader;
}

QString ObsidianConfigReader::getConfigPath() const
{
    // macOS: ~/Library/Application Support/obsidian/obsidian.json
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return configPath + "/obsidian/obsidian.json";
}

bool ObsidianConfigReader::hasObsidianConfig() const
{
    return QFile::exists(getConfigPath());
}

QList<ObsidianVaultInfo> ObsidianConfigReader::getObsidianVaults() const
{
    QList<ObsidianVaultInfo> result;
    
    QString configPath = getConfigPath();
    QFile file(configPath);
    
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return result;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return result;
    }
    
    QJsonObject root = doc.object();
    QJsonObject vaults = root["vaults"].toObject();
    
    for (auto it = vaults.begin(); it != vaults.end(); ++it) {
        QJsonObject vaultObj = it->toObject();
        
        ObsidianVaultInfo info;
        info.id = it.key();
        info.path = vaultObj["path"].toString();
        info.timestamp = vaultObj["ts"].toVariant().toLongLong();
        info.isOpen = vaultObj["open"].toBool(false);
        
        // Check if path exists
        QDir dir(info.path);
        info.exists = dir.exists();
        
        // Check if it's a valid Obsidian vault (has .obsidian directory)
        info.isValid = dir.exists(".obsidian");
        
        // Get vault name
        if (info.exists && info.isValid) {
            info.name = getVaultName(info.path);
        } else {
            // Use directory name as fallback
            info.name = dir.dirName();
        }
        
        result.append(info);
    }
    
    // Sort by timestamp (most recent first)
    std::sort(result.begin(), result.end(), [](const ObsidianVaultInfo& a, const ObsidianVaultInfo& b) {
        return a.timestamp > b.timestamp;
    });
    
    return result;
}

ObsidianVaultInfo ObsidianConfigReader::getVaultByPath(const QString& path) const
{
    QList<ObsidianVaultInfo> vaults = getObsidianVaults();
    
    // Normalize path for comparison
    QString normalizedPath = QDir::cleanPath(path);
    
    for (const ObsidianVaultInfo& vault : vaults) {
        QString vaultPath = QDir::cleanPath(vault.path);
        if (vaultPath == normalizedPath) {
            return vault;
        }
    }
    
    // Return empty info if not found
    return ObsidianVaultInfo();
}

QString ObsidianConfigReader::getVaultName(const QString& path) const
{
    // Try to read from .obsidian/app.json
    QDir vaultDir(path);
    QString appJsonPath = vaultDir.filePath(".obsidian/app.json");
    
    QFile appJsonFile(appJsonPath);
    if (appJsonFile.exists() && appJsonFile.open(QIODevice::ReadOnly)) {
        QByteArray data = appJsonFile.readAll();
        appJsonFile.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            
            // Try "vault" field first (newer Obsidian versions)
            if (obj.contains("vault") && obj["vault"].isObject()) {
                QJsonObject vaultObj = obj["vault"].toObject();
                if (vaultObj.contains("name") && !vaultObj["name"].toString().isEmpty()) {
                    return vaultObj["name"].toString();
                }
            }
            
            // Try "name" field directly
            if (obj.contains("name") && !obj["name"].toString().isEmpty()) {
                return obj["name"].toString();
            }
        }
    }
    
    // Fallback to directory name
    return vaultDir.dirName();
}

QString ObsidianConfigReader::generateVaultId()
{
    // Initialize random seed
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)));
        seeded = true;
    }
    
    // Generate 16-character hex string
    QString id;
    for (int i = 0; i < 16; ++i) {
        id += QString::number(rand() % 16, 16);
    }
    return id;
}

QString ObsidianConfigReader::registerVault(const QString& path)
{
    QString configPath = getConfigPath();
    QString normalizedPath = QDir::cleanPath(path);
    
    // Read existing config or create new one
    QJsonObject root;
    QJsonObject vaults;
    
    QFile file(configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            root = doc.object();
            vaults = root["vaults"].toObject();
        }
    }
    
    // Check if vault already exists (by path match)
    QString existingId;
    for (auto it = vaults.begin(); it != vaults.end(); ++it) {
        QJsonObject vaultObj = it->toObject();
        QString vaultPath = QDir::cleanPath(vaultObj["path"].toString());
        if (vaultPath == normalizedPath) {
            existingId = it.key();
            break;
        }
    }
    
    qint64 currentTs = QDateTime::currentDateTime().toSecsSinceEpoch();
    
    if (!existingId.isEmpty()) {
        // Update existing vault entry
        QJsonObject vaultObj = vaults[existingId].toObject();
        vaultObj["ts"] = currentTs;
        vaultObj["open"] = true;
        vaults[existingId] = vaultObj;
    } else {
        // Generate new vault ID and add entry
        QString newId = generateVaultId();
        QJsonObject newVault;
        newVault["path"] = normalizedPath;
        newVault["ts"] = currentTs;
        newVault["open"] = true;
        vaults[newId] = newVault;
        existingId = newId;
    }
    
    // Update root object
    root["vaults"] = vaults;
    if (!root.contains("frame")) {
        root["frame"] = "custom";
    }
    
    // Write config back
    // Ensure config directory exists
    QDir configDir = QFileInfo(configPath).dir();
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument newDoc(root);
        file.write(newDoc.toJson(QJsonDocument::Indented));
        file.close();
        return existingId;
    }
    
    return QString();
}

bool ObsidianConfigReader::isObsidianRunning() const
{
    // Check if Obsidian process is running
    // macOS: pgrep -x Obsidian
    QProcess process;
    process.start("pgrep", QStringList{"-x", "Obsidian"});
    process.waitForFinished(3000);
    
    return process.exitCode() == 0;
}