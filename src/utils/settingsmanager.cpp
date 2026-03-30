#include "settingsmanager.h"

SettingsManager* SettingsManager::instance()
{
    static SettingsManager manager;
    return &manager;
}

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings("R2MO", "ObsidianManager")
{
}

QString SettingsManager::obsidianAppPath() const
{
    return m_settings.value("obsidianAppPath", "/Applications/Obsidian.app").toString();
}

void SettingsManager::setObsidianAppPath(const QString& path)
{
    m_settings.setValue("obsidianAppPath", path);
}

QString SettingsManager::language() const
{
    return m_settings.value("language").toString();
}

void SettingsManager::setLanguage(const QString& languageCode)
{
    m_settings.setValue("language", languageCode);
}

int SettingsManager::theme() const
{
    return m_settings.value("theme", 0).toInt(); // 0 = Light, 1 = Dark
}

void SettingsManager::setTheme(int theme)
{
    m_settings.setValue("theme", theme);
}

QByteArray SettingsManager::windowGeometry() const
{
    return m_settings.value("windowGeometry").toByteArray();
}

void SettingsManager::setWindowGeometry(const QByteArray& geometry)
{
    m_settings.setValue("windowGeometry", geometry);
}

QString SettingsManager::lastVaultPath() const
{
    return m_settings.value("lastVaultPath").toString();
}

void SettingsManager::setLastVaultPath(const QString& path)
{
    m_settings.setValue("lastVaultPath", path);
}

void SettingsManager::sync()
{
    m_settings.sync();
}