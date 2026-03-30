#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QFont>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Theme {
        Light,
        Dark
    };

    static ThemeManager* instance();
    
    void initialize();
    void setTheme(Theme theme);
    Theme currentTheme() const;
    QString currentThemeName() const;
    
    QString lightStyle() const;
    QString darkStyle() const;
    QString currentStyle() const;
    
    // Font helpers
    static QFont uiFont();           // Helvetica Neue for UI
    static QFont monoFont();         // JetBrains Mono for English text (one size larger)

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() = default;
    
    Q_DISABLE_COPY(ThemeManager)
    
    Theme m_currentTheme;
    QMap<Theme, QString> m_themeNames;
};

#endif // THEMEMANAGER_H