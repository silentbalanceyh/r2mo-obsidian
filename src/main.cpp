#include <QApplication>
#include <QSettings>
#include <QFont>
#include <QFontDatabase>
#include "mainwindow.h"
#include "i18n/translationmanager.h"
#include "theme/thememanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Obsidian Manager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("R2MO");
    
    // Load embedded MesloLGS NF fonts
    QFontDatabase::addApplicationFont(":/fonts/MesloLGS-NF-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/MesloLGS-NF-Bold.ttf");
    
    // Set default font to PingFang SC for Chinese (14px)
    QFont defaultFont = QFont("PingFang SC", 14);
    app.setFont(defaultFont);

    // Initialize theme system
    ThemeManager::instance()->initialize();
    
    // Initialize translation system
    TranslationManager::instance()->initialize();
    
    // Load saved language preference, default to Chinese
    QSettings settings;
    QString savedLang = settings.value("language").toString();
    if (savedLang.isEmpty()) {
        savedLang = "zh_CN";  // Default to Chinese
    }
    TranslationManager::instance()->switchLanguage(savedLang);

    MainWindow window;
    window.setWindowTitle(QApplication::translate("MainWindow", "Obsidian Manager - Vault Center"));
    window.resize(1680, 1050);
    window.show();

    return app.exec();
}
