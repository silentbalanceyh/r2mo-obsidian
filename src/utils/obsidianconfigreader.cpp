#include "obsidianconfigreader.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDateTime>

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