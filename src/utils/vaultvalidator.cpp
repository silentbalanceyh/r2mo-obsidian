#include "vaultvalidator.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

VaultValidator& VaultValidator::instance()
{
    static VaultValidator validator;
    return validator;
}

bool VaultValidator::isValidVault(const QString& path) const
{
    return hasObsidianConfig(path);
}

bool VaultValidator::hasObsidianConfig(const QString& path) const
{
    QDir dir(path);
    return dir.exists(".obsidian");
}

bool VaultValidator::hasR2moConfig(const QString& path) const
{
    QDir dir(path);
    return dir.exists(".r2mo");
}

QString VaultValidator::validationMessage(const QString& path) const
{
    if (isValidVault(path)) {
        QString msg = "Valid Obsidian Vault";
        if (hasR2moConfig(path)) {
            msg += " with R2MO configuration";
        }
        return msg;
    }
    return "Not a valid Obsidian Vault (missing .obsidian directory)";
}

QString VaultValidator::getVaultName(const QString& path) const
{
    // Try to read vault name from .obsidian/app.json
    QDir vaultDir(path);
    QString appJsonPath = vaultDir.filePath(".obsidian/app.json");
    
    QFile appJsonFile(appJsonPath);
    if (appJsonFile.exists() && appJsonFile.open(QIODevice::ReadOnly)) {
        QByteArray data = appJsonFile.readAll();
        appJsonFile.close();
        
        if (!data.isEmpty()) {
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
    }
    
    // Fallback to directory name
    return vaultDir.dirName();
}