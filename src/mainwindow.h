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
#include <QLineEdit>
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
    void onRenameVault();
    void onOpenInObsidian();
    void onSettings();
    void onVaultSelected(QListWidgetItem *item);
    void onVaultDoubleClicked(QListWidgetItem *item);
    void onVaultContextMenu(const QPoint &pos);
    void onVaultItemChanged(QListWidgetItem *item);
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
    void onPreviewEditClicked();
    void onPreviewTitleEditFinished();
    void onPreviewTitleReturnPressed();
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
    QLabel *m_previewLabel;
    QWidget *m_previewHeader;
    QLabel *m_previewTitle;
    QPushButton *m_previewEditBtn;
    QLineEdit *m_previewTitleEdit;
    QTextEdit *m_previewPane;
    QString m_currentPreviewPath;

    // Modules (not owned)
    VaultModel *m_vaultModel;
    SettingsManager *m_settingsManager;
    VaultValidator *m_vaultValidator;
    
    // For tracking rename
    QString m_editingVaultPath;
    
    // Config path
    QString m_configPath;
};

#endif // MAINWINDOW_H