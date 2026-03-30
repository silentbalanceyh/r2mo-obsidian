#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>

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
    
    void sync();

private:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() = default;
    
    Q_DISABLE_COPY(SettingsManager)
    
    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H