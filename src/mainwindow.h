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
#include <QTreeWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTabWidget>
#include <QFrame>
#include <QGridLayout>
#include "theme/thememanager.h"

// Forward declarations
class VaultModel;
class SettingsManager;
class VaultValidator;
struct R2moSubProject;

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
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupConnections();
    void updateVaultList();
    void updatePreviewPane(const QString& name, const QString& path);
    void openVaultInObsidian(const QString& vaultPath);
    void onPreviewEditClicked();
    void onPreviewOpenClicked();
    void onPreviewTitleEditFinished();
    void onPreviewTitleReturnPressed();
    void onTaskItemDoubleClicked(QTreeWidgetItem* item, int column);
    void drawProjectGraph(const QList<R2moSubProject>& projects);
    void buildTaskTree(const QList<R2moSubProject>& projects);
    void setOverviewEmptyState(bool empty);
    void clearOverviewGrid(QGridLayout *layout);
    void addOverviewRow(QGridLayout *layout, int row, const QString& label, const QString& value,
                        const QString& valueColor = QString(), const QString& suffixHtml = QString());
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
    QPushButton *m_previewOpenBtn;
    QLineEdit *m_previewTitleEdit;
    QTabWidget *m_tabWidget;
    QWidget *m_overviewTab;
    QLabel *m_overviewEmptyLabel;
    QWidget *m_overviewContent;
    QLabel *m_overviewPathLabel;
    QFrame *m_vaultStatsCard;
    QLabel *m_vaultStatsHeader;
    QWidget *m_vaultStatsBody;
    QGridLayout *m_vaultStatsGrid;
    QFrame *m_r2moStatsCard;
    QLabel *m_r2moStatsHeader;
    QWidget *m_r2moStatsBody;
    QGridLayout *m_r2moStatsGrid;
    QWidget *m_tasksTab;
    QWidget *m_graphTab;
    QGraphicsView *m_graphView;
    QGraphicsScene *m_graphScene;
    QTreeWidget *m_taskTree;
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
