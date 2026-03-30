#include <QApplication>
#include <QSettings>
#include "mainwindow.h"
#include "i18n/translationmanager.h"
#include "theme/thememanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Obsidian Manager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("R2MO");

    // Initialize theme system
    ThemeManager::instance()->initialize();
    
    // Initialize translation system
    TranslationManager::instance()->initialize();
    
    // Load saved language preference
    QSettings settings;
    QString savedLang = settings.value("language").toString();
    if (!savedLang.isEmpty()) {
        TranslationManager::instance()->switchLanguage(savedLang);
    }

    MainWindow window;
    window.setWindowTitle(QApplication::translate("MainWindow", "Obsidian Manager - Vault Center"));
    window.resize(1680, 900);
    window.show();

    return app.exec();
}