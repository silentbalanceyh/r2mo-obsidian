#include "settingsmanager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

QList<SpecialMonitorSource> SettingsManager::specialMonitorSources() const
{
    QList<SpecialMonitorSource> sources;
    const QByteArray raw = m_settings.value("specialMonitorSources").toByteArray();
    if (raw.isEmpty()) {
        return sources;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) {
        return sources;
    }

    const QJsonArray array = doc.array();
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        SpecialMonitorSource source;
        source.providerName = object.value("providerName").toString();
        source.baseUrl = object.value("baseUrl").toString();
        source.tokenKey = object.value("tokenKey").toString();
        if (source.baseUrl.trimmed().isEmpty() || source.tokenKey.trimmed().isEmpty()) {
            continue;
        }
        if (source.providerName.trimmed().isEmpty()) {
            source.providerName = SpecialMonitorFetcher::defaultProviderName(source.baseUrl);
        }
        sources.append(source);
    }

    return sources;
}

void SettingsManager::setSpecialMonitorSources(const QList<SpecialMonitorSource>& sources)
{
    QJsonArray array;
    for (const SpecialMonitorSource& source : sources) {
        if (source.baseUrl.trimmed().isEmpty() || source.tokenKey.trimmed().isEmpty()) {
            continue;
        }
        QJsonObject object;
        object.insert("providerName", source.providerName);
        object.insert("baseUrl", SpecialMonitorFetcher::normalizeBaseUrl(source.baseUrl));
        object.insert("tokenKey", source.tokenKey.trimmed());
        array.append(object);
    }
    m_settings.setValue("specialMonitorSources", QJsonDocument(array).toJson(QJsonDocument::Compact));
    m_settings.sync();
}

QByteArray SettingsManager::monitorBoardHeaderState() const
{
    return m_settings.value("monitorBoardHeaderState.v3").toByteArray();
}

void SettingsManager::setMonitorBoardHeaderState(const QByteArray& state)
{
    m_settings.setValue("monitorBoardHeaderState.v3", state);
}

QByteArray SettingsManager::specialMonitorHeaderState() const
{
    return m_settings.value("specialMonitorHeaderState.v3").toByteArray();
}

void SettingsManager::setSpecialMonitorHeaderState(const QByteArray& state)
{
    m_settings.setValue("specialMonitorHeaderState.v3", state);
}

void SettingsManager::sync()
{
    m_settings.sync();
}
