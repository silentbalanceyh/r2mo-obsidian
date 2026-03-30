#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>

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