#include "vaultvalidator.h"
#include <QDir>

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