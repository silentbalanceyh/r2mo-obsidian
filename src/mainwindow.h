#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QToolBar>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include "theme/thememanager.h"

// Forward declarations
class VaultModel;
class SettingsManager;
class VaultValidator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddVault();
    void onRemoveVault();
    void onOpenInObsidian();
    void onSettings();
    void onVaultSelected(QListWidgetItem *item);
    void onAbout();
    void onLanguageChanged(const QString& languageCode);
    void onLanguageButtonClicked(int id);
    void onThemeChanged(ThemeManager::Theme theme);
    void onThemeToggle();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupConnections();
    void updateVaultList();
    void updatePreviewPane(const QString& name, const QString& path);
    void openVaultInObsidian(const QString& vaultPath);
    void retranslateUi();
    void updateLanguageButtons();
    void updateThemeToggleIcon();

    // Toolbar
    QToolBar *m_toolBar;
    QPushButton *m_themeBtn;
    QButtonGroup *m_langGroup;
    QPushButton *m_btnZh;
    QPushButton *m_btnEn;
    
    // Central widget
    QSplitter *m_splitter;
    QLabel *m_vaultLabel;
    QListWidget *m_vaultList;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_openBtn;
    QLabel *m_previewLabel;
    QTextEdit *m_previewPane;

    // Modules (not owned)
    VaultModel *m_vaultModel;
    SettingsManager *m_settingsManager;
    VaultValidator *m_vaultValidator;
    
    // Config path
    QString m_configPath;
};

#endif // MAINWINDOW_H