#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QList>
#include "specialmonitorfetcher.h"

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager* instance();
    
    // Obsidian App Path
    QString obsidianAppPath() const;
    void setObsidianAppPath(const QString& path);
    
    // Language
    QString language() const;
    void setLanguage(const QString& languageCode);
    
    // Theme
    int theme() const;
    void setTheme(int theme);
    
    // Window geometry
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);
    
    // Last opened vault
    QString lastVaultPath() const;
    void setLastVaultPath(const QString& path);

    // Special monitor sources
    QList<SpecialMonitorSource> specialMonitorSources() const;
    void setSpecialMonitorSources(const QList<SpecialMonitorSource>& sources);

    // Header states
    QByteArray monitorBoardHeaderState() const;
    void setMonitorBoardHeaderState(const QByteArray& state);
    QByteArray specialMonitorHeaderState() const;
    void setSpecialMonitorHeaderState(const QByteArray& state);
    
    // Countdown reference date
    QString countdownReferenceDate() const;
    void setCountdownReferenceDate(const QString& date);

    void sync();

private:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() = default;
    
    Q_DISABLE_COPY(SettingsManager)
    
    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
