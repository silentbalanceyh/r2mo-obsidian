#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Obsidian Manager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("R2MO");

    MainWindow window;
    window.setWindowTitle("Obsidian Manager - Vault Center");
    window.resize(1200, 800);
    window.show();

    return app.exec();
}