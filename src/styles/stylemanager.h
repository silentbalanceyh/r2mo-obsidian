#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>

class StyleManager
{
public:
    static StyleManager& instance();
    
    QString modernStyle() const;
    QString buttonStyle() const;
    QString listStyle() const;
    QString menuStyle() const;
    QString scrollBarStyle() const;
    
private:
    StyleManager() = default;
    ~StyleManager() = default;
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
    
    QString loadStyleFromFile(const QString& filename) const;
};

#endif // STYLEMANAGER_H