#ifndef VAULTVALIDATOR_H
#define VAULTVALIDATOR_H

#include <QString>

class VaultValidator
{
public:
    static VaultValidator& instance();
    
    bool isValidVault(const QString& path) const;
    bool hasObsidianConfig(const QString& path) const;
    bool hasR2moConfig(const QString& path) const;
    
    QString validationMessage(const QString& path) const;

private:
    VaultValidator() = default;
    ~VaultValidator() = default;
    
    Q_DISABLE_COPY(VaultValidator)
};

#endif // VAULTVALIDATOR_H