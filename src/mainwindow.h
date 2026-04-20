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
#include <QTimer>
#include <QFutureWatcher>
#include <QStringList>
#include "theme/thememanager.h"
#include "utils/gitscanner.h"
#include "utils/aitoolscanner.h"
#include "utils/r2moscanner.h"
#include "utils/sessionscanner.h"

// Forward declarations
class VaultModel;
class SettingsManager;
class VaultValidator;
struct Vault;

struct SwimlaneVaultData {
    QString vaultName;
    QString vaultPath;
    GitStatusInfo gitStatus;
    QList<AIToolInfo> aiTools;
    struct LaneRow {
        QString name;
        QString projectPath;
        QString r2moPath;
        int historicalCount;
        QList<TaskInfo> queueTasks;
        bool isParent;
        GitStatusInfo gitStatus;
        QList<LaneRow> children;
    };
    QList<LaneRow> rows;
};

struct SwimlaneScanData {
    QList<SwimlaneVaultData> vaults;
    int globalMaxQueue;
};

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
    void onSwimlane();
    void onSwimlaneRefresh();
    void onMonitorBoard();
    void onMonitorRefresh();
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
    QString resolveObsidianOpenPath(const QString& vaultPath) const;
    void onPreviewEditClicked();
    void onPreviewOpenClicked();
    void onPreviewTitleEditFinished();
    void onPreviewTitleReturnPressed();
    void onTaskItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onAIToolItemDoubleClicked(QTreeWidgetItem* item, int column);
    void drawProjectGraph(const QList<R2moSubProject>& projects);
    void buildTaskTree(const QList<R2moSubProject>& projects);
    void setOverviewEmptyState(bool empty);
    void clearOverviewGrid(QGridLayout *layout);
    void addOverviewRow(QGridLayout *layout, int row, const QString& label, const QString& value,
                        const QString& valueColor = QString(), const QString& suffixHtml = QString());
    void buildAIToolsTab(const QString& vaultPath);
    void buildAIToolsTree(const QList<R2moSubProject>& projects);
    bool addAIToolsToItem(QTreeWidgetItem* parentItem, const QList<AIToolInfo>& tools);
    void showHistoricalTasksDialog(const QString& r2moPath, const QString& projectName);
    void retranslateUi();
    void updateLanguageButtons();
    void updateThemeToggleIcon();
    void updateToolbarIcons();
    QIcon createSwimlaneIcon(const QColor &baseColor) const;
    QIcon createMonitorIcon(const QColor &baseColor) const;
    QIcon createHomeIcon(const QColor &baseColor) const;
    void openSwimlaneTab();
    void addSwimlaneCloseButton(int tabIndex);
    void refreshSwimlaneAsync();
    SwimlaneScanData collectSwimlaneData();
    QWidget* buildSwimlaneView(const SwimlaneScanData& data);
    QWidget* buildSwimlaneView();
    void openMonitorTab();
    void addMonitorCloseButton(int tabIndex);
    void refreshMonitorAsync();
    QList<QPair<QString, QString>> collectAllProjectPaths();
    QWidget* buildMonitorView(const QList<ProjectMonitorData>& data);
    void updateMonitorTableColumns(QTreeWidget *tree);
    void replaceMonitorContent(QWidget *newContent, bool preserveCurrentTab);
    bool updateMonitorStatusCells(const QList<ProjectMonitorData>& data);
    QString monitorRowKey(const QString& projectPath, const SessionInfo& session) const;
    void updateMonitorStatusLabel(QLabel *label, SessionStatus status) const;
    void showSessionDetailDialog(const SessionInfo& session);
    void syncVaultOrderFromList();
    void openMonitorTarget(QTreeWidgetItem *row);
    void invalidateMonitorView(bool refreshIfOpen);
    bool activateTerminalWindow(qint64 pid);

    // Toolbar
    QToolBar *m_toolBar;
    QPushButton *m_themeBtn;
    QButtonGroup *m_langGroup;
    QPushButton *m_btnZh;
    QPushButton *m_btnEn;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_swimlaneBtn;
    QPushButton *m_monitorBtn;
    
    // Main content tabs
    QTabWidget *m_mainTabWidget;
    QWidget *m_homeTabContent;
    QWidget *m_swimlaneTabContent;
    QWidget *m_monitorTabContent;
    
    // Central widget
    QSplitter *m_splitter;
    QLabel *m_vaultLabel;
    QListWidget *m_vaultList;
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
    QWidget *m_aiToolsTab;
    QLabel *m_aiToolsEmptyLabel;
    QTreeWidget *m_aiToolsTree;
    int m_swimlaneTabIndex;
    QWidget *m_swimlaneView;
    QWidget *m_cachedSwimlaneWidget;
    QTimer *m_swimlaneRefreshTimer;
    QFutureWatcher<SwimlaneScanData> *m_swimlaneScanWatcher;
    bool m_swimlaneRefreshing;
    QTimer *m_loadingProgressTimer;
    QLabel *m_loadingProgressLabel;
    int m_loadingProgressStep;
    QString m_currentPreviewPath;
    int m_monitorTabIndex;
    QWidget *m_monitorView;
    QWidget *m_cachedMonitorWidget;
    QTimer *m_monitorRefreshTimer;
    QFutureWatcher<QList<ProjectMonitorData>> *m_monitorScanWatcher;
    bool m_monitorRefreshing;
    QLabel *m_monitorProgressLabel;
    int m_monitorProgressStep;

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
