#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddVault();
    void onRemoveVault();
    void onOpenVault();
    void onOpenInObsidian();
    void onSettings();
    void onVaultSelected(QListWidgetItem *item);
    void onAbout();

private:
    void setupMenuBar();
    void setupCentralWidget();
    void loadVaults();
    void saveVaults();
    void updateVaultList();
    QString getObsidianAppPath();
    void openVaultInObsidian(const QString &vaultPath);

    // UI components
    QSplitter *m_splitter;
    QListWidget *m_vaultList;
    QTextEdit *m_previewPane;

    // Data
    QJsonArray m_vaults;
    QString m_configPath;
    QString m_obsidianAppPath;
};

#endif // MAINWINDOW_H