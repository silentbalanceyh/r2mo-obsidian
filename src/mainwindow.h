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
#include <QTableWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTabWidget>
#include <QFrame>
#include <QGridLayout>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QHash>
#include <QPixmap>
#include <QSet>
#include <QStringList>
#include "theme/thememanager.h"
#include "utils/gitscanner.h"
#include "utils/sshremoteexecutor.h"
#include "utils/aitoolscanner.h"
#include "utils/r2moscanner.h"
#include "utils/sessionscanner.h"
#include "utils/specialmonitorfetcher.h"

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

struct TimedSwimlaneCache {
    SwimlaneScanData data;
    QDateTime capturedAt;
};

struct TimedGitStatusCache {
    GitStatusInfo data;
    QDateTime capturedAt;
};

struct TimedAIToolCache {
    QList<AIToolInfo> data;
    QDateTime capturedAt;
};

struct TimedProjectPathCache {
    QList<QPair<QString, QString>> data;
    QDateTime capturedAt;
};

struct TimedSpecialMonitorCache {
    QList<SpecialMonitorSnapshot> data;
    QDateTime capturedAt;
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
    struct RemoteDirectoryEntry {
        QString path;
        QString displayName;
    };

    struct PreviewProjectCache {
        QString vaultPath;
        QList<R2moSubProject> projects;
        bool hasR2moConfig = false;
        bool loaded = false;
        bool projectsLoaded = false;
        int projectCount = 0;
        int totalQueue = 0;
        int totalHistorical = 0;
    };

    struct LocalPreviewStats {
        QString vaultPath;
        int files = 0;
        int folders = 0;
        int hiddenItems = 0;
        bool hasObsidianConfig = false;
        bool pathExists = false;
    };

    struct MonitorRefreshResult {
        QList<ProjectMonitorData> localData;
        QList<ProjectMonitorData> remoteData;
        bool localOnly = false;
    };

    struct RemoteDirectoryFetchResult {
        QString vaultPath;
        QString basePath;
        QList<RemoteDirectoryEntry> entries;
        QString errorMessage;
    };

    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupConnections();
    void updateVaultList();
    void updatePreviewPane(const QString& name, const QString& path);
    void openVaultInObsidian(const QString& vaultPath);
    QString resolveObsidianOpenPath(const QString& vaultPath) const;
    Vault vaultByPath(const QString& path) const;
    QString vaultStatusBadgeText(const Vault& vault) const;
    QColor vaultStatusBadgeColor(const Vault& vault) const;
    QListWidgetItem* addVaultListGroupHeader(const QString& title);
    QList<Vault> localVaults() const;
    QList<Vault> remoteVaults() const;
    void addRemoteRepository();
    QString selectRemoteDirectory(const Vault& vault, const QString& startPath) const;
    QList<RemoteDirectoryEntry> fetchRemoteDirectories(const Vault& vault, const QString& basePath, QString* errorMessage = nullptr) const;
    void refreshRemoteConnectivityStatuses(bool force = false);
    void checkRemoteVaultConnectivityAsync(const QString& vaultPath);
    void applyRemoteConnectivityResult(const QString& vaultPath, bool connected, const QString& errorText);
    QList<ProjectMonitorData> collectRemoteMonitorData() const;
    void onPreviewEditClicked();
    void onPreviewOpenClicked();
    void onPreviewTitleEditFinished();
    void onPreviewTitleReturnPressed();
    void onTaskItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onAIToolItemDoubleClicked(QTreeWidgetItem* item, int column);
    void drawProjectGraph(const QList<R2moSubProject>& projects);
    void buildTaskTree(const QList<R2moSubProject>& projects);
    void ensureOverviewInitialized();
    void ensureTasksTabInitialized();
    void ensureGraphTabInitialized();
    void ensureAIToolsTabInitialized();
    void ensureSwimlaneInfrastructureInitialized();
    void ensurePreviewAsyncInfrastructureInitialized();
    void ensureMonitorInfrastructureInitialized();
    void ensureRemoteConnectivityInfrastructureInitialized();
    void ensureSpecialMonitorInfrastructureInitialized();
    void applyLocalPreviewStats(const LocalPreviewStats& stats);
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
    void updateMemoryUsageLabel();
    QIcon createSwimlaneIcon(const QColor &baseColor) const;
    QIcon createMonitorIcon(const QColor &baseColor) const;
    QIcon createHomeIcon(const QColor &baseColor) const;
    void openSwimlaneTab();
    void addSwimlaneCloseButton(int tabIndex);
    void refreshSwimlaneAsync();
    SwimlaneScanData collectSwimlaneData();
    QWidget* buildSwimlaneView(const SwimlaneScanData& data);
    QWidget* buildSwimlaneView();
    GitStatusInfo cachedGitStatusForPath(const QString& path);
    QList<AIToolInfo> cachedAIToolsForPath(const QString& path);
    QList<QPair<QString, QString>> cachedProjectPaths();
    void pruneScanCaches();
    void openMonitorTab();
    void addMonitorCloseButton(int tabIndex);
    void refreshMonitorAsync();
    void refreshMonitorLocalStatusAsync();
    void startMonitorRefresh(bool localOnly);
    QList<QPair<QString, QString>> collectAllProjectPaths();
    QWidget* buildMonitorView(const QList<ProjectMonitorData>& data);
    QWidget* buildSpecialMonitorPanel();
    void updateSpecialMonitorPanelSizing();
    void refreshSpecialMonitorAsync(bool force = true);
    void setSpecialMonitorActionsEnabled(bool enabled);
    void setSpecialMonitorRefreshLoading(bool loading);
    void setSpecialMonitorRowsLoading(bool loading);
    void updateSpecialMonitorTable(const QList<SpecialMonitorSnapshot>& snapshots);
    void updateSpecialMonitorTableColumns();
    void addSpecialMonitorSource();
    void editSpecialMonitorSource();
    void removeSpecialMonitorSource();
    void updateMonitorTableColumns(QTreeWidget *tree);
    void replaceMonitorContent(QWidget *newContent, bool preserveCurrentTab);
    void setMonitorRefreshEnabled(bool enabled);
    void setMonitorRefreshLoading(bool loading);
    void setMonitorRowsLoading(bool loading);
    QString formatSessionRuntime(qint64 runtimeSeconds) const;
    bool updateMonitorStatusCells(const QList<ProjectMonitorData>& data);
    bool updateMonitorLocalStatusCells(const QList<ProjectMonitorData>& localData);
    QString monitorRowKey(const QString& projectPath, const SessionInfo& session) const;
    void copyMonitorRestoreCommand(const QString& command);
    void updateMonitorStatusLabel(QWidget *label, SessionStatus status) const;
    void showSessionDetailDialog(const SessionInfo& session);
    void syncVaultOrderFromList();
    void openMonitorTarget(QTreeWidgetItem *row);
    void invalidateMonitorView(bool refreshIfOpen);
    void applyMonitorRefreshResult(const MonitorRefreshResult& result);
    void rebuildMonitorArtifactWatchers();
    QStringList collectMonitorArtifactWatchPaths(const QList<QPair<QString, QString>>& projects) const;
    void setMonitorArtifactWatchEnabled(bool enabled);
    void upsertMonitorArtifactWatchPath(const QString& path);
    void clearMonitorArtifactWatchPaths();
    QTreeWidgetItem* createMonitorTreeRow(const ProjectMonitorData& data, QTreeWidget *tree) const;
    void requestPreviewProjectCache(const QString& vaultPath, bool includeProjects = false);
    void requestRemoteDirectoryPreview(const Vault& vault);
    bool activateTerminalWindow(qint64 pid);
    void ensurePreviewTabContent(int tabIndex);
    void clearPreviewTabContent();
    PreviewProjectCache loadPreviewProjectCache(const QString& vaultPath, bool includeProjects) const;

    // Toolbar
    QToolBar *m_toolBar;
    QPushButton *m_themeBtn;
    QLabel *m_memoryUsageLabel;
    QButtonGroup *m_langGroup;
    QPushButton *m_btnZh;
    QPushButton *m_btnEn;
    QPushButton *m_addBtn;
    QPushButton *m_addRemoteBtn;
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
    TimedSwimlaneCache m_swimlaneDataCache;
    QHash<QString, TimedGitStatusCache> m_gitStatusCache;
    QHash<QString, TimedAIToolCache> m_aiToolCache;
    TimedProjectPathCache m_projectPathCache;
    QTimer *m_swimlaneRefreshTimer;
    QFutureWatcher<SwimlaneScanData> *m_swimlaneScanWatcher;
    bool m_swimlaneRefreshing;
    QTimer *m_loadingProgressTimer;
    QLabel *m_loadingProgressLabel;
    int m_loadingProgressStep;
    QString m_currentPreviewPath;
    PreviewProjectCache m_previewProjectCache;
    QFutureWatcher<LocalPreviewStats> *m_localPreviewStatsWatcher;
    QFutureWatcher<PreviewProjectCache> *m_previewProjectCacheWatcher;
    QFutureWatcher<RemoteDirectoryFetchResult> *m_remoteDirectoryPreviewWatcher;
    int m_monitorTabIndex;
    QWidget *m_monitorView;
    QTimer *m_monitorRefreshTimer;
    QTimer *m_monitorArtifactDebounceTimer;
    QFileSystemWatcher *m_monitorArtifactWatcher;
    QFutureWatcher<MonitorRefreshResult> *m_monitorScanWatcher;
    bool m_monitorRefreshing;
    bool m_monitorLocalStatusRefreshPending;
    QSet<QString> m_monitorArtifactWatchPaths;
    QLabel *m_monitorProgressLabel;
    int m_monitorProgressStep;
    QTimer *m_memoryUsageTimer;
    QTimer *m_remoteConnectivityTimer;
    QWidget *m_specialMonitorPanel;
    QTableWidget *m_specialMonitorTable;
    QFutureWatcher<QList<SpecialMonitorSnapshot>> *m_specialMonitorScanWatcher;
    bool m_specialMonitorRefreshing;
    QLabel *m_specialMonitorStatusLabel;
    QTimer *m_specialMonitorRefreshTimer;
    TimedSpecialMonitorCache m_specialMonitorDataCache;
    QSplitter *m_monitorVerticalSplitter;
    int m_specialMonitorPanelStretchId;
    bool m_specialMonitorPanelUserResized;

    // Modules (not owned)
    VaultModel *m_vaultModel;
    SettingsManager *m_settingsManager;
    VaultValidator *m_vaultValidator;
    SshRemoteExecutor m_sshRemoteExecutor;
    
    // For tracking rename
    QString m_editingVaultPath;
    
    // Config path
    QString m_configPath;
};

#endif // MAINWINDOW_H
