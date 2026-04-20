#include "mainwindow.h"
#include "theme/thememanager.h"
#include "models/vaultmodel.h"
#include "utils/settingsmanager.h"
#include "utils/vaultvalidator.h"
#include "utils/obsidianconfigreader.h"
#include "utils/r2moscanner.h"
#include "utils/aitoolscanner.h"
#include "utils/sessionscanner.h"
#include "utils/gitscanner.h"
#include "i18n/translationmanager.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QHeaderView>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QDesktopServices>
#include <QUrl>
#include <QThread>
#include <QGraphicsDropShadowEffect>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QTabWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSpacerItem>
#include <QStatusBar>
#include <QPixmap>
#include <QToolButton>
#include <QProgressBar>
#include <QClipboard>
#include <QtConcurrent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace {
struct SpecialMonitorProviderOption {
    QString label;
    QString baseUrl;
};

QList<SpecialMonitorProviderOption> specialMonitorProviders()
{
    return {
        {QStringLiteral("💻 PP Coding"), QStringLiteral("https://code.ppchat.vip")}
    };
}

QString specialMonitorSourceKey(const QString& baseUrl, const QString& tokenKey)
{
    return QStringLiteral("%1|%2")
        .arg(SpecialMonitorFetcher::normalizeBaseUrl(baseUrl),
             tokenKey.trimmed());
}
}

MainWindow::MainWindow(QWidget *parent)
     : QMainWindow(parent)
     , m_toolBar(nullptr)
     , m_themeBtn(nullptr)
     , m_langGroup(nullptr)
     , m_btnZh(nullptr)
     , m_btnEn(nullptr)
     , m_addBtn(nullptr)
     , m_removeBtn(nullptr)
     , m_swimlaneBtn(nullptr)
      , m_monitorBtn(nullptr)
     , m_splitter(nullptr)
     , m_vaultLabel(nullptr)
     , m_vaultList(nullptr)
     , m_previewLabel(nullptr)
     , m_previewHeader(nullptr)
     , m_previewTitle(nullptr)
     , m_previewEditBtn(nullptr)
     , m_previewOpenBtn(nullptr)
     , m_previewTitleEdit(nullptr)
     , m_tabWidget(nullptr)
     , m_overviewTab(nullptr)
     , m_overviewEmptyLabel(nullptr)
     , m_overviewContent(nullptr)
     , m_overviewPathLabel(nullptr)
     , m_vaultStatsCard(nullptr)
     , m_vaultStatsHeader(nullptr)
     , m_vaultStatsBody(nullptr)
     , m_vaultStatsGrid(nullptr)
     , m_r2moStatsCard(nullptr)
     , m_r2moStatsHeader(nullptr)
     , m_r2moStatsBody(nullptr)
     , m_r2moStatsGrid(nullptr)
     , m_tasksTab(nullptr)
     , m_graphTab(nullptr)
     , m_graphView(nullptr)
     , m_graphScene(nullptr)
     , m_taskTree(nullptr)
     , m_aiToolsTab(nullptr)
     , m_aiToolsEmptyLabel(nullptr)
     , m_aiToolsTree(nullptr)
, m_swimlaneTabIndex(-1)
      , m_swimlaneView(nullptr)
      , m_cachedSwimlaneWidget(nullptr)
      , m_swimlaneRefreshTimer(nullptr)
      , m_swimlaneScanWatcher(nullptr)
      , m_swimlaneRefreshing(false)
      , m_loadingProgressTimer(nullptr)
      , m_loadingProgressLabel(nullptr)
      , m_loadingProgressStep(0)
      , m_monitorTabIndex(-1)
      , m_monitorView(nullptr)
      , m_cachedMonitorWidget(nullptr)
      , m_monitorRefreshTimer(nullptr)
      , m_monitorScanWatcher(nullptr)
      , m_monitorRefreshing(false)
      , m_monitorProgressLabel(nullptr)
      , m_monitorProgressStep(0)
      , m_specialMonitorPanel(nullptr)
      , m_specialMonitorTable(nullptr)
      , m_specialMonitorScanWatcher(nullptr)
      , m_specialMonitorRefreshing(false)
      , m_specialMonitorStatusLabel(nullptr)
      , m_monitorVerticalSplitter(nullptr)
      , m_specialMonitorPanelStretchId(0)
      , m_specialMonitorPanelUserResized(false)
     , m_vaultModel(nullptr)
     , m_settingsManager(nullptr)
     , m_vaultValidator(nullptr)
     , m_editingVaultPath()
     , m_configPath()
{
    // Apply theme style
    setStyleSheet(ThemeManager::instance()->currentStyle());

    // Initialize modules
    m_settingsManager = SettingsManager::instance();
    m_vaultValidator = &VaultValidator::instance();
    
    // Create vault model
    m_vaultModel = new VaultModel(this);
    
    // Config path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_configPath = configDir + "/vaults.json";
    
    // Load vaults
    m_vaultModel->load(m_configPath);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupConnections();
    updateVaultList();
    updateLanguageButtons();
    updateToolbarIcons();
    
    // Set default window size: 1680 x 900
    resize(1680, 900);
    
    // Restore window geometry if saved
    QByteArray geometry = m_settingsManager->windowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
}

MainWindow::~MainWindow()
{
    // Save vaults
    m_vaultModel->save(m_configPath);
    
    // Save window geometry
    m_settingsManager->setWindowGeometry(saveGeometry());
    m_settingsManager->sync();
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));

    QAction *addVaultAct = new QAction(tr("Add Vault..."), this);
    addVaultAct->setShortcut(QKeySequence("Ctrl+N"));
    connect(addVaultAct, &QAction::triggered, this, &MainWindow::onAddVault);
    fileMenu->addAction(addVaultAct);

    QAction *removeVaultAct = new QAction(tr("Remove Vault"), this);
    removeVaultAct->setShortcut(QKeySequence("Ctrl+Delete"));
    connect(removeVaultAct, &QAction::triggered, this, &MainWindow::onRemoveVault);
    fileMenu->addAction(removeVaultAct);

    fileMenu->addSeparator();

    QAction *openVaultAct = new QAction(tr("Open in Obsidian"), this);
    openVaultAct->setShortcut(QKeySequence("Ctrl+O"));
    connect(openVaultAct, &QAction::triggered, this, &MainWindow::onOpenInObsidian);
    fileMenu->addAction(openVaultAct);

    fileMenu->addSeparator();

    QAction *exitAct = new QAction(tr("Exit"), this);
    exitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAct, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAct);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
    QAction *settingsAct = new QAction(tr("Settings..."), this);
    settingsAct->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettings);
    editMenu->addAction(settingsAct);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("Help"));
    QAction *aboutAct = new QAction(tr("About"), this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAct);
}

void MainWindow::setupToolBar()
{
    // Create toolbar
    m_toolBar = addToolBar(tr("Toolbar"));
    m_toolBar->setMovable(false);
    
    // Create left-side container for Add/Remove as ButtonGroup + Divider + Swimlane
    QWidget *leftControls = new QWidget();
    leftControls->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *leftLayout = new QHBoxLayout(leftControls);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);
    
    // Add/Remove as segmented control (like 中|En)
    QWidget *addRemoveGroup = new QWidget();
    addRemoveGroup->setObjectName("addRemoveGroup");
    QHBoxLayout *arLayout = new QHBoxLayout(addRemoveGroup);
    arLayout->setContentsMargins(0, 0, 0, 0);
    arLayout->setSpacing(0);
    
    m_addBtn = new QPushButton();
    m_addBtn->setText("+");
    m_addBtn->setObjectName("addBtnLeft");
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setToolTip(tr("Add Vault"));
    arLayout->addWidget(m_addBtn);
    
    m_removeBtn = new QPushButton();
    m_removeBtn->setText("−");
    m_removeBtn->setObjectName("removeBtnRight");
    m_removeBtn->setCursor(Qt::PointingHandCursor);
    m_removeBtn->setToolTip(tr("Remove Vault"));
    arLayout->addWidget(m_removeBtn);
    
    leftLayout->addWidget(addRemoveGroup);
    
    // Divider
    QFrame *divider1 = new QFrame();
    divider1->setFrameShape(QFrame::VLine);
    divider1->setFrameShadow(QFrame::Sunken);
    divider1->setFixedSize(1, 24);
    leftLayout->addWidget(divider1);
    
    // Swimlane button (dynamic icon)
    m_swimlaneBtn = new QPushButton();
    m_swimlaneBtn->setObjectName("swimlaneBtn");
    m_swimlaneBtn->setCursor(Qt::PointingHandCursor);
    m_swimlaneBtn->setToolTip(tr("Swimlane View"));
    m_swimlaneBtn->setIconSize(QSize(20, 20));
    m_swimlaneBtn->setFixedSize(32, 32);
    leftLayout->addWidget(m_swimlaneBtn);
    
    // Monitor Board button (dynamic icon) - directly adjacent to swimlane
    m_monitorBtn = new QPushButton();
    m_monitorBtn->setObjectName("monitorBtn");
    m_monitorBtn->setCursor(Qt::PointingHandCursor);
    m_monitorBtn->setToolTip(tr("Monitor Board"));
    m_monitorBtn->setIconSize(QSize(20, 20));
    m_monitorBtn->setFixedSize(32, 32);
    leftLayout->addWidget(m_monitorBtn);
    
    m_toolBar->addWidget(leftControls);
    
    // Add spacer to push buttons to the right
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);
    
    // Create a container widget for right-side controls
    QWidget *rightControls = new QWidget();
    QHBoxLayout *rightLayout = new QHBoxLayout(rightControls);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    rightLayout->setSpacing(12);
    
    // Theme toggle button (square icon with moon/sun)
    m_themeBtn = new QPushButton();
    m_themeBtn->setObjectName("themeBtn");
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    updateThemeToggleIcon();
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    rightLayout->addWidget(m_themeBtn);
    
    // Language button group container (中|En as segmented control)
    QWidget *langGroupWidget = new QWidget();
    langGroupWidget->setObjectName("langGroupWidget");
    QHBoxLayout *langLayout = new QHBoxLayout(langGroupWidget);
    langLayout->setContentsMargins(0, 0, 0, 0);
    langLayout->setSpacing(0);  // No gap between buttons
    
    // Create button group for exclusive selection
    m_langGroup = new QButtonGroup(this);
    m_langGroup->setExclusive(true);
    
    // Chinese button (left side of segmented control)
    m_btnZh = new QPushButton("中");
    m_btnZh->setCheckable(true);
    m_btnZh->setObjectName("langBtnLeft");
    m_btnZh->setToolTip("简体中文");
    m_btnZh->setFixedSize(32, 32);
    m_btnZh->setCursor(Qt::PointingHandCursor);
    m_langGroup->addButton(m_btnZh, 0);
    langLayout->addWidget(m_btnZh);
    
    // English button (right side of segmented control)
    m_btnEn = new QPushButton("En");
    m_btnEn->setCheckable(true);
    m_btnEn->setObjectName("langBtnRight");
    m_btnEn->setToolTip("English");
    m_btnEn->setFixedSize(32, 32);
    m_btnEn->setCursor(Qt::PointingHandCursor);
    m_langGroup->addButton(m_btnEn, 1);
    langLayout->addWidget(m_btnEn);
    
    rightLayout->addWidget(langGroupWidget);
    
    // Add the right controls widget to toolbar
    m_toolBar->addWidget(rightControls);
    
    // Connect language button group
    connect(m_langGroup, &QButtonGroup::idClicked, 
            this, &MainWindow::onLanguageButtonClicked);
    
    // Connect Add/Remove buttons
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddVault);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveVault);
    
    // Connect Swimlane button
    connect(m_swimlaneBtn, &QPushButton::clicked, this, &MainWindow::onSwimlane);
    
    // Connect Monitor Board button
    connect(m_monitorBtn, &QPushButton::clicked, this, &MainWindow::onMonitorBoard);
    
    // Set initial language selection
    updateLanguageButtons();
}

void MainWindow::updateThemeToggleIcon()
{
    // Use moon/sun emoji icons
    if (ThemeManager::instance()->currentTheme() == ThemeManager::Light) {
        // Light mode - show moon icon (click to switch to dark)
        m_themeBtn->setText("🌙");
        m_themeBtn->setToolTip(tr("Switch to Dark Mode"));
    } else {
        // Dark mode - show sun icon (click to switch to light)
        m_themeBtn->setText("☀️");
        m_themeBtn->setToolTip(tr("Switch to Light Mode"));
    }
}

void MainWindow::updateToolbarIcons()
{
    QColor baseColor = (ThemeManager::instance()->currentTheme() == ThemeManager::Light)
        ? QColor("#333333") : QColor("#f5f5f7");
    
    m_swimlaneBtn->setIcon(createSwimlaneIcon(baseColor));
    m_monitorBtn->setIcon(createMonitorIcon(baseColor));
    
    int homeIdx = m_mainTabWidget->indexOf(m_homeTabContent);
    if (homeIdx >= 0) {
        m_mainTabWidget->setTabIcon(homeIdx, createHomeIcon(baseColor));
    }
}

QIcon MainWindow::createSwimlaneIcon(const QColor &baseColor) const
{
    const int sz = 24;
    const qreal dpr = devicePixelRatioF();
    QPixmap pixmap(qRound(sz * dpr), qRound(sz * dpr));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    
    QPen pen(baseColor, 1.5);
    pen.setCosmetic(true);
    
    // Outer frame
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(1.5, 1.5, 21, 21), 2.5, 2.5);
    
    // Vertical bars
    p.setPen(Qt::NoPen);
    p.setBrush(baseColor);
    p.drawRect(QRectF(4.5, 13, 3, 7));
    p.drawRect(QRectF(10, 8, 3, 12));
    p.drawRect(QRectF(15.5, 10.5, 3, 9.5));
    
    // Dot markers
    p.setBrush(QColor("#007aff"));
    p.drawEllipse(QRectF(4, 9.5, 4, 4));
    p.setBrush(QColor("#34c759"));
    p.drawEllipse(QRectF(9.5, 5, 4, 4));
    p.setBrush(QColor("#ff9500"));
    p.drawEllipse(QRectF(15, 7.5, 4, 4));
    
    p.end();
    return QIcon(pixmap);
}

QIcon MainWindow::createMonitorIcon(const QColor &baseColor) const
{
    const int sz = 24;
    const qreal dpr = devicePixelRatioF();
    QPixmap pixmap(qRound(sz * dpr), qRound(sz * dpr));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    
    QPen pen(baseColor, 1.5);
    pen.setCosmetic(true);
    
    // Screen frame
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(1.5, 2.5, 21, 17), 2.5, 2.5);
    
    // Stand
    p.setPen(Qt::NoPen);
    p.setBrush(baseColor);
    p.drawRect(QRectF(9, 19.5, 6, 1.5));
    p.drawRect(QRectF(7, 21, 10, 1));
    
    // Status dots
    p.setBrush(QColor("#ff3b30"));
    p.drawEllipse(QRectF(4.5, 5, 3.5, 3.5));
    p.setBrush(QColor("#ff9500"));
    p.drawEllipse(QRectF(10, 5, 3.5, 3.5));
    p.setBrush(QColor("#34c759"));
    p.drawEllipse(QRectF(15.5, 5, 3.5, 3.5));
    
    // Session cards
    p.setBrush(QColor("#34c759"));
    p.drawRoundedRect(QRectF(4, 10.5, 6.5, 3), 0.8, 0.8);
    p.setBrush(QColor("#007aff"));
    p.drawRoundedRect(QRectF(12.5, 10.5, 6.5, 3), 0.8, 0.8);
    p.setBrush(QColor("#007aff"));
    p.drawRoundedRect(QRectF(4, 14.5, 6.5, 3), 0.8, 0.8);
    p.setBrush(QColor("#34c759"));
    p.drawRoundedRect(QRectF(12.5, 14.5, 6.5, 3), 0.8, 0.8);
    
    p.end();
    return QIcon(pixmap);
}

QIcon MainWindow::createHomeIcon(const QColor &baseColor) const
{
    const int sz = 20;
    const qreal dpr = devicePixelRatioF();
    QPixmap pixmap(qRound(sz * dpr), qRound(sz * dpr));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    
    QPen pen(baseColor, 1.2);
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    
    // Roof
    QPainterPath roof;
    roof.moveTo(10, 2);
    roof.lineTo(18, 9);
    roof.lineTo(2, 9);
    roof.closeSubpath();
    p.setPen(pen);
    p.setBrush(baseColor);
    p.drawPath(roof);
    
    // Body (house wall)
    p.setPen(Qt::NoPen);
    p.setBrush(baseColor);
    p.drawRect(QRectF(4, 9, 12, 8));
    
    // Door cutout (transparent)
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setBrush(Qt::transparent);
    p.drawRect(QRectF(7.5, 12, 5, 5));
    
    p.end();
    return QIcon(pixmap);
}

void MainWindow::updateLanguageButtons()
{
    QString currentLang = TranslationManager::instance()->currentLanguage();
    
    // Block signals to prevent flickering
    m_btnZh->blockSignals(true);
    m_btnEn->blockSignals(true);
    
    if (currentLang == "zh_CN") {
        m_btnZh->setChecked(true);
    } else {
        m_btnEn->setChecked(true);
    }
    
    m_btnZh->blockSignals(false);
    m_btnEn->blockSignals(false);
}

void MainWindow::onLanguageButtonClicked(int id)
{
    QString langCode = (id == 0) ? "zh_CN" : "en_US";
    TranslationManager::instance()->switchLanguage(langCode);
    m_settingsManager->setLanguage(langCode);
    invalidateMonitorView(m_monitorTabContent != nullptr);
    updateSpecialMonitorPanelSizing();
}

void MainWindow::setupCentralWidget()
{
    // Create central widget with margin
    QWidget *centralWidget = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(8);

    // Create main tab widget for top-level navigation
    QTabWidget *mainTabWidget = new QTabWidget;
    mainTabWidget->setObjectName("mainTabs");
    mainTabWidget->setTabsClosable(true);
    mainTabWidget->setStyleSheet(
        "QTabWidget#mainTabs { background: white; }"
        "QTabWidget#mainTabs::pane { border: 1px solid #e0e0e0; border-radius: 4px; background: white; }"
        "QTabWidget#mainTabs::tab-bar { alignment: left; }"
        "QTabBar::tab { padding: 8px 20px; margin-right: 4px; border: 1px solid #e0e0e0; border-bottom: none; border-radius: 4px 4px 0 0; background: #f5f5f7; }"
        "QTabWidget#mainTabs QTabBar::tab:selected { background: white; color: #007aff; border-bottom: 1px solid white; }"
        "QTabWidget#mainTabs QTabBar::tab:!selected { background: #e8e8ed; color: #86868b; }"
        "QTabBar::close-button { subcontrol-position: right; margin: 4px; }"
    );
    
    // HOME tab content: splitter with vault list and preview
    QWidget *homeTabContent = new QWidget;
    QVBoxLayout *homeTabLayout = new QVBoxLayout(homeTabContent);
    homeTabLayout->setContentsMargins(0, 8, 0, 0);
    homeTabLayout->setSpacing(8);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(12);
    m_splitter->setChildrenCollapsible(false);

    // Left: Vault list panel
    QWidget *leftPanel = new QWidget;
    leftPanel->setObjectName("leftPanel");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    leftLayout->setSpacing(12);

    // Section header
    m_vaultLabel = new QLabel(tr("Vaults"));
    m_vaultLabel->setObjectName("sectionLabel");
    m_vaultLabel->setFixedHeight(36);
    leftLayout->addWidget(m_vaultLabel);

    // Vault list with shadow
    m_vaultList = new QListWidget;
    m_vaultList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_vaultList->setMinimumWidth(320);
    m_vaultList->setSpacing(1);
    m_vaultList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_vaultList->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_vaultList->setDragEnabled(true);
    m_vaultList->setAcceptDrops(true);
    m_vaultList->setDropIndicatorShown(true);
    m_vaultList->setDefaultDropAction(Qt::MoveAction);
    m_vaultList->setDragDropMode(QAbstractItemView::InternalMove);
    m_vaultList->setStyleSheet("QListWidget::item { min-height: 30px; padding: 5px 10px; border-radius: 3px; } QListWidget::item:selected { background: #007aff; color: white; }");

    QGraphicsDropShadowEffect *listShadow = new QGraphicsDropShadowEffect;
    listShadow->setBlurRadius(20);
    listShadow->setColor(QColor(0, 0, 0, 30));
    listShadow->setOffset(0, 4);
    m_vaultList->setGraphicsEffect(listShadow);

    leftLayout->addWidget(m_vaultList, 1);

    m_splitter->addWidget(leftPanel);

    // Right: Tab widget with preview tabs
    QWidget *rightPanel = new QWidget;
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    rightLayout->setSpacing(12);

    // Section header
    m_previewLabel = new QLabel(tr("Preview"));
    m_previewLabel->setObjectName("sectionLabel");
    m_previewLabel->setFixedHeight(36);
    rightLayout->addWidget(m_previewLabel);

    // Preview header with title and buttons
    m_previewHeader = new QWidget;
    QHBoxLayout *headerLayout = new QHBoxLayout(m_previewHeader);
    headerLayout->setContentsMargins(0, 0, 0, 8);
    headerLayout->setSpacing(4);
    
    m_previewEditBtn = new QPushButton("✏️");
    m_previewEditBtn->setObjectName("editBtn");
    m_previewEditBtn->setFixedSize(24, 24);
    m_previewEditBtn->setCursor(Qt::PointingHandCursor);
    m_previewEditBtn->setToolTip(tr("Rename"));
    m_previewEditBtn->hide();
    headerLayout->addWidget(m_previewEditBtn);
    
    m_previewTitle = new QLabel;
    m_previewTitle->setObjectName("previewTitle");
    m_previewTitle->setStyleSheet("font-size: 18px; font-weight: 600;");
    m_previewTitle->setText(tr("Select a vault..."));
    headerLayout->addWidget(m_previewTitle, 1);
    
    m_previewOpenBtn = new QPushButton(tr("Open"));
    m_previewOpenBtn->setObjectName("primaryBtn");
    m_previewOpenBtn->setFixedHeight(28);
    m_previewOpenBtn->setCursor(Qt::PointingHandCursor);
    m_previewOpenBtn->setToolTip(tr("Open in Obsidian"));
    m_previewOpenBtn->hide();
    headerLayout->addWidget(m_previewOpenBtn);
    
    m_previewTitleEdit = new QLineEdit;
    m_previewTitleEdit->setObjectName("previewTitleEdit");
    m_previewTitleEdit->setStyleSheet("QLineEdit#previewTitleEdit { font-size: 18px; font-weight: 600; background: white; border: 2px solid #007aff; border-radius: 3px; padding: 4px 8px; }");
    m_previewTitleEdit->hide();
    headerLayout->addWidget(m_previewTitleEdit, 1);
    
    rightLayout->addWidget(m_previewHeader);

    // Create tab widget for preview tabs
    m_tabWidget = new QTabWidget;
    m_tabWidget->setObjectName("previewTabs");
    // Preview inner tabs are not user-closeable
    m_tabWidget->setTabsClosable(false);
    m_tabWidget->setStyleSheet(
        "QTabWidget#previewTabs { border: none; background: white; }"
        "QTabWidget#previewTabs::pane { border: none; background: white; }"
        "QTabWidget#previewTabs::tab-bar { alignment: left; }"
        "QTabBar::tab { padding: 8px 20px; margin-right: 0px; border: 1px solid #e0e0e0; border-bottom: none; border-radius: 0px; background: #f5f5f7; }"
        "QTabBar::tab:selected { background: white; color: #007aff; border-bottom: 1px solid white; }"
        "QTabBar::tab:!selected { background: #e8e8ed; color: #86868b; margin-top: 2px; }"
        "QTabBar::close-button { subcontrol-position: right; margin: 4px; }"
    );
    
    // Tab 0: HOME (概览) - non-closeable
    m_overviewTab = new QWidget;
    QVBoxLayout *overviewLayout = new QVBoxLayout(m_overviewTab);
    overviewLayout->setContentsMargins(8, 8, 8, 8);
    overviewLayout->setSpacing(8);
    
    m_overviewEmptyLabel = new QLabel(tr("Select a vault to view details..."));
    m_overviewEmptyLabel->setAlignment(Qt::AlignCenter);
    m_overviewEmptyLabel->setWordWrap(true);
    m_overviewEmptyLabel->setStyleSheet("QLabel { color: #86868b; font-size: 15px; padding: 24px; }");
    overviewLayout->addWidget(m_overviewEmptyLabel, 1);

    m_overviewContent = new QWidget;
    QVBoxLayout *overviewContentLayout = new QVBoxLayout(m_overviewContent);
    overviewContentLayout->setContentsMargins(0, 0, 0, 0);
    overviewContentLayout->setSpacing(8);

    m_overviewPathLabel = new QLabel;
    m_overviewPathLabel->setWordWrap(true);
    m_overviewPathLabel->setStyleSheet("QLabel { color: #ff3b30; font-size: 16px; background: transparent; }");
    overviewContentLayout->addWidget(m_overviewPathLabel);

    m_vaultStatsCard = new QFrame;
    m_vaultStatsCard->setStyleSheet("QFrame { background: transparent; border: none; }");
    QVBoxLayout *vaultCardLayout = new QVBoxLayout(m_vaultStatsCard);
    vaultCardLayout->setContentsMargins(0, 0, 0, 0);
    vaultCardLayout->setSpacing(0);
    m_vaultStatsHeader = new QLabel(tr("Vault Statistics"));
    m_vaultStatsHeader->setStyleSheet("QLabel { background: #f5f5f7; color: #1d1d1f; font-size: 15px; font-weight: 600; padding: 14px 20px; border-bottom: 2px solid #e0e0e0; }");
    vaultCardLayout->addWidget(m_vaultStatsHeader);
    m_vaultStatsBody = new QWidget;
    m_vaultStatsBody->setStyleSheet("QWidget { background: transparent; }");
    m_vaultStatsGrid = new QGridLayout(m_vaultStatsBody);
    m_vaultStatsGrid->setContentsMargins(0, 0, 0, 0);
    m_vaultStatsGrid->setHorizontalSpacing(0);
    m_vaultStatsGrid->setVerticalSpacing(0);
    m_vaultStatsGrid->setColumnStretch(0, 1);
    m_vaultStatsGrid->setColumnStretch(1, 1);
    vaultCardLayout->addWidget(m_vaultStatsBody);
    overviewContentLayout->addWidget(m_vaultStatsCard);

    m_r2moStatsCard = new QFrame;
    m_r2moStatsCard->setStyleSheet("QFrame { background: transparent; border: none; }");
    QVBoxLayout *r2moCardLayout = new QVBoxLayout(m_r2moStatsCard);
    r2moCardLayout->setContentsMargins(0, 0, 0, 0);
    r2moCardLayout->setSpacing(0);
    m_r2moStatsHeader = new QLabel(tr("R2MO Statistics"));
    m_r2moStatsHeader->setStyleSheet("QLabel { background: #f5f5f7; color: #1d1d1f; font-size: 15px; font-weight: 600; padding: 14px 20px; border-bottom: 2px solid #e0e0e0; }");
    r2moCardLayout->addWidget(m_r2moStatsHeader);
    m_r2moStatsBody = new QWidget;
    m_r2moStatsBody->setStyleSheet("QWidget { background: transparent; }");
    m_r2moStatsGrid = new QGridLayout(m_r2moStatsBody);
    m_r2moStatsGrid->setContentsMargins(0, 0, 0, 0);
    m_r2moStatsGrid->setHorizontalSpacing(0);
    m_r2moStatsGrid->setVerticalSpacing(0);
    m_r2moStatsGrid->setColumnStretch(0, 1);
    m_r2moStatsGrid->setColumnStretch(1, 1);
    r2moCardLayout->addWidget(m_r2moStatsBody);
    overviewContentLayout->addWidget(m_r2moStatsCard);

    overviewContentLayout->addStretch(1);
    overviewLayout->addWidget(m_overviewContent, 1);
    setOverviewEmptyState(true);
    m_tabWidget->addTab(m_overviewTab, tr("概览"));
    m_tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    
    // Tab 2: Project Tasks (项目任务)
    m_tasksTab = new QWidget;
    QVBoxLayout *tasksLayout = new QVBoxLayout(m_tasksTab);
    tasksLayout->setContentsMargins(0, 0, 0, 0);
    tasksLayout->setSpacing(0);
    
    m_taskTree = new QTreeWidget;
    m_taskTree->setHeaderLabel(tr("Tasks"));
    m_taskTree->setIndentation(22);
    m_taskTree->setUniformRowHeights(false);
    m_taskTree->setStyleSheet("QTreeWidget { border: none; background: white; } QTreeWidget::item { padding: 6px 4px; } QTreeWidget::item:selected { background: #007aff; color: white; }");
    m_taskTree->setAlternatingRowColors(false);
    m_taskTree->setRootIsDecorated(true);  // Show guide lines
    m_taskTree->setItemsExpandable(true);   // Allow expand/collapse
    m_taskTree->setAnimated(true);          // Animate expand/collapse
    
    tasksLayout->addWidget(m_taskTree, 1);
    m_tabWidget->addTab(m_tasksTab, tr("项目任务"));
    
    // Tab 3: Structure Graph (结构图) - fill entire page
    m_graphTab = new QWidget;
    QVBoxLayout *graphLayout = new QVBoxLayout(m_graphTab);
    graphLayout->setContentsMargins(0, 0, 0, 0);
    graphLayout->setSpacing(0);
    
    m_graphScene = new QGraphicsScene(this);
    m_graphView = new QGraphicsView(m_graphScene);
    m_graphView->setRenderHint(QPainter::Antialiasing);
    m_graphView->setStyleSheet("QGraphicsView { border: none; background: white; }");
    m_graphView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_graphView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_graphView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_graphView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_graphView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    m_graphView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_graphView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_graphView->setBackgroundBrush(QBrush(QColor("#fafafa")));
    m_graphView->installEventFilter(this);  // Install event filter for zoom/pan
    
    graphLayout->addWidget(m_graphView, 1);
    m_tabWidget->addTab(m_graphTab, tr("结构图"));
    
    // Tab 4: AI Tools (AI工具)
    m_aiToolsTab = new QWidget;
    QVBoxLayout *aiToolsLayout = new QVBoxLayout(m_aiToolsTab);
    aiToolsLayout->setContentsMargins(0, 0, 0, 0);
    aiToolsLayout->setSpacing(0);
    
    m_aiToolsEmptyLabel = new QLabel(tr("Select a vault with AI tool configurations..."));
    m_aiToolsEmptyLabel->setAlignment(Qt::AlignCenter);
    m_aiToolsEmptyLabel->setWordWrap(true);
    m_aiToolsEmptyLabel->setStyleSheet("QLabel { color: #86868b; font-size: 15px; padding: 24px; }");
    aiToolsLayout->addWidget(m_aiToolsEmptyLabel, 1);
    
    m_aiToolsTree = new QTreeWidget;
    m_aiToolsTree->setHeaderLabel(tr("AI Tools"));
    m_aiToolsTree->setIndentation(22);
    m_aiToolsTree->setUniformRowHeights(false);
    m_aiToolsTree->setStyleSheet("QTreeWidget { border: none; background: white; } QTreeWidget::item { padding: 6px 4px; } QTreeWidget::item:selected { background: #007aff; color: white; }");
    m_aiToolsTree->setAlternatingRowColors(false);
    m_aiToolsTree->setRootIsDecorated(true);
    m_aiToolsTree->setItemsExpandable(true);
    m_aiToolsTree->setAnimated(true);
    m_aiToolsTree->setVisible(false);
    aiToolsLayout->addWidget(m_aiToolsTree, 1);
    
    m_tabWidget->addTab(m_aiToolsTab, tr("AI工具"));

    rightLayout->addWidget(m_tabWidget, 1);

    m_splitter->addWidget(rightPanel);

    m_splitter->setSizes({400, 1280});
    homeTabLayout->addWidget(m_splitter, 1);

    // Create main tab widget to hold HOME and Swimlane views
    m_mainTabWidget = new QTabWidget;
    m_mainTabWidget->setObjectName("mainTabs");
    m_mainTabWidget->setTabsClosable(false);
    m_mainTabWidget->setStyleSheet(
        "QTabWidget#mainTabs { background: transparent; border: none; }"
        "QTabWidget#mainTabs::pane { border: none; background: transparent; }"
        "QTabWidget#mainTabs::tab-bar { alignment: left; }"
        "QTabWidget#mainTabs QTabBar::tab { padding: 6px 16px; margin-right: 2px; background: #f0f0f0; border: 1px solid #d0d0d0; border-bottom: none; border-radius: 4px 4px 0 0; }"
        "QTabWidget#mainTabs QTabBar::tab:selected { background: white; color: #007aff; border-color: #007aff; }"
        "QTabWidget#mainTabs QTabBar::tab:!selected { background: #e8e8e8; color: #666; }"
        "QTabWidget#mainTabs QTabBar::close-button { width: 14px; height: 14px; subcontrol-position: right; }"
    );
    m_homeTabContent = homeTabContent;
    m_mainTabWidget->addTab(m_homeTabContent, tr("HOME"));
    m_mainTabWidget->setTabIcon(0, createHomeIcon(
        (ThemeManager::instance()->currentTheme() == ThemeManager::Light)
            ? QColor("#333333") : QColor("#f5f5f7")));
    
    m_swimlaneTabContent = nullptr;
    m_monitorTabContent = nullptr;
    
    mainLayout->addWidget(m_mainTabWidget, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupConnections()
{
    connect(m_vaultList, &QListWidget::itemClicked, this, &MainWindow::onVaultSelected);
    connect(m_vaultList, &QListWidget::itemDoubleClicked, this, &MainWindow::onVaultDoubleClicked);
    connect(m_vaultList, &QListWidget::customContextMenuRequested, this, &MainWindow::onVaultContextMenu);
    connect(m_vaultList, &QListWidget::itemChanged, this, &MainWindow::onVaultItemChanged);
    connect(m_vaultList->model(), &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex&, int, int, const QModelIndex&, int) {
                QTimer::singleShot(0, this, [this]() {
                    syncVaultOrderFromList();
                });
            });
    connect(m_previewEditBtn, &QPushButton::clicked, this, &MainWindow::onPreviewEditClicked);
    connect(m_previewOpenBtn, &QPushButton::clicked, this, &MainWindow::onPreviewOpenClicked);
    connect(m_previewTitleEdit, &QLineEdit::editingFinished, this, &MainWindow::onPreviewTitleEditFinished);
    connect(m_previewTitleEdit, &QLineEdit::returnPressed, this, &MainWindow::onPreviewTitleReturnPressed);
    connect(m_taskTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onTaskItemDoubleClicked);
    connect(m_aiToolsTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onAIToolItemDoubleClicked);
    connect(m_vaultModel, &VaultModel::modelChanged, this, &MainWindow::updateVaultList);
    connect(TranslationManager::instance(), &TranslationManager::languageChanged, 
            this, &MainWindow::onLanguageChanged);
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
    // Swimlane refresh timer (10 seconds)
    m_swimlaneRefreshTimer = new QTimer(this);
    m_swimlaneRefreshTimer->setInterval(10000);
    connect(m_swimlaneRefreshTimer, &QTimer::timeout, this, &MainWindow::onSwimlaneRefresh);
    m_swimlaneRefreshTimer->start(); // Start immediately on app launch
    
    // Loading progress animation timer
    m_loadingProgressTimer = new QTimer(this);
    m_loadingProgressTimer->setInterval(300);
    connect(m_loadingProgressTimer, &QTimer::timeout, this, [this]() {
        if (m_loadingProgressLabel) {
            m_loadingProgressStep = (m_loadingProgressStep + 1) % 4;
            QString dots = QString(".").repeated(m_loadingProgressStep + 1);
            static const QStringList steps = {
                QT_TR_NOOP("Scanning Git repositories"),
                QT_TR_NOOP("Scanning AI tools"),
                QT_TR_NOOP("Loading task queues"),
                QT_TR_NOOP("Building swimlane view")
            };
            QString stepText = tr(steps[m_loadingProgressStep % steps.size()].toUtf8().constData());
            m_loadingProgressLabel->setText(QString("%1%2").arg(stepText, dots));
        }
    });
    
    // Monitor refresh timer (15 seconds)
    m_monitorRefreshTimer = new QTimer(this);
    m_monitorRefreshTimer->setInterval(2000);
    connect(m_monitorRefreshTimer, &QTimer::timeout, this, &MainWindow::onMonitorRefresh);
    m_monitorRefreshTimer->start();
    
    // Monitor scan watcher for async refresh
    m_monitorScanWatcher = new QFutureWatcher<QList<ProjectMonitorData>>(this);
    connect(m_monitorScanWatcher, &QFutureWatcher<QList<ProjectMonitorData>>::finished, this, [this]() {
        if (m_monitorRefreshing) {
            QList<ProjectMonitorData> data = m_monitorScanWatcher->result();
            if (!updateMonitorStatusCells(data)) {
                QWidget *newWidget = buildMonitorView(data);
                replaceMonitorContent(newWidget, true);
            }
            m_monitorRefreshing = false;
        }
    });

    m_specialMonitorScanWatcher = new QFutureWatcher<QList<SpecialMonitorSnapshot>>(this);
    connect(m_specialMonitorScanWatcher, &QFutureWatcher<QList<SpecialMonitorSnapshot>>::finished, this, [this]() {
        if (m_specialMonitorRefreshing) {
            updateSpecialMonitorTable(m_specialMonitorScanWatcher->result());
            m_specialMonitorRefreshing = false;
        }
    });
    
    // Swimlane scan watcher for async refresh
    m_swimlaneScanWatcher = new QFutureWatcher<SwimlaneScanData>(this);
    connect(m_swimlaneScanWatcher, &QFutureWatcher<SwimlaneScanData>::finished, this, [this]() {
        if (m_swimlaneRefreshing) {
            m_loadingProgressTimer->stop();
            m_loadingProgressStep = 0;
            
            SwimlaneScanData data = m_swimlaneScanWatcher->result();
            QWidget *newWidget = buildSwimlaneView(data);
            
            // Always update cache with fresh data
            m_cachedSwimlaneWidget = newWidget;
            
            // Update visible tab if showing swimlane
            if (m_swimlaneTabContent && m_mainTabWidget->indexOf(m_swimlaneTabContent) >= 0) {
                int idx = m_mainTabWidget->indexOf(m_swimlaneTabContent);
                
                // If swimlane tab is currently visible, update it
                if (m_mainTabWidget->currentIndex() == idx) {
                    m_mainTabWidget->removeTab(idx);
                    m_swimlaneTabContent = newWidget;
                    int newIdx = m_mainTabWidget->addTab(m_swimlaneTabContent, tr("Swimlane"));
                    addSwimlaneCloseButton(newIdx);
                    m_mainTabWidget->setCurrentIndex(newIdx);
                }
            }
            m_swimlaneRefreshing = false;
        }
    });
}

void MainWindow::updateVaultList()
{
    const QString preferredPath = m_currentPreviewPath.isEmpty()
        ? m_settingsManager->lastVaultPath()
        : m_currentPreviewPath;

    const QSignalBlocker blocker(m_vaultList);
    m_vaultList->clear();
    
    QList<Vault> vaults = m_vaultModel->vaults();
    QListWidgetItem *selectedItem = nullptr;
    for (const Vault& v : vaults) {
        QListWidgetItem *item = new QListWidgetItem(v.name);
        item->setData(Qt::UserRole, v.path);
        item->setToolTip(v.path);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_vaultList->addItem(item);
        if (!preferredPath.isEmpty() && v.path == preferredPath) {
            selectedItem = item;
        }
    }

    if (!selectedItem && !preferredPath.isEmpty()) {
        for (int i = 0; i < m_vaultList->count(); ++i) {
            QListWidgetItem *item = m_vaultList->item(i);
            if (item && item->data(Qt::UserRole).toString() == preferredPath) {
                selectedItem = item;
                break;
            }
        }
    }

    if (selectedItem) {
        m_vaultList->setCurrentItem(selectedItem);
        updatePreviewPane(selectedItem->text(), selectedItem->data(Qt::UserRole).toString());
    } else if (vaults.isEmpty()) {
        m_currentPreviewPath.clear();
        setOverviewEmptyState(true);
    }

    invalidateMonitorView(m_monitorTabContent != nullptr);
}

void MainWindow::syncVaultOrderFromList()
{
    QStringList orderedPaths;
    orderedPaths.reserve(m_vaultList->count());
    for (int i = 0; i < m_vaultList->count(); ++i) {
        QListWidgetItem *item = m_vaultList->item(i);
        if (!item) {
            continue;
        }
        orderedPaths.append(item->data(Qt::UserRole).toString());
    }

    if (orderedPaths.size() != m_vaultModel->count()) {
        return;
    }

    m_vaultModel->reorderVaults(orderedPaths);
    m_vaultModel->save(m_configPath);
}

void MainWindow::invalidateMonitorView(bool refreshIfOpen)
{
    if (m_cachedMonitorWidget && m_cachedMonitorWidget != m_monitorTabContent) {
        m_cachedMonitorWidget->deleteLater();
    }

    if (!m_monitorTabContent) {
        m_cachedMonitorWidget = nullptr;
        return;
    }

    if (refreshIfOpen && !m_monitorRefreshing) {
        refreshMonitorAsync();
        return;
    }

    m_cachedMonitorWidget = m_monitorTabContent;
}

void MainWindow::onAddVault()
{
    // Create selection dialog with tree view
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Project Directory"));
    dialog.setMinimumSize(1200, 600);
    dialog.resize(1400, 700);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    // Header
    QLabel* headerLabel = new QLabel(tr("Select a project directory:"));
    headerLabel->setStyleSheet("font-size: 15px; font-weight: 600;");
    layout->addWidget(headerLabel);
    
    // Tree widget
    QTreeWidget* treeWidget = new QTreeWidget();
    treeWidget->setHeaderLabels(QStringList() << tr("Name") << tr("Status") << tr("Path"));
    treeWidget->setRootIsDecorated(false);
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeWidget->setExpandsOnDoubleClick(false);
    treeWidget->setIconSize(QSize(14, 14));
    treeWidget->setIndentation(18);
    treeWidget->viewport()->setProperty("isProjectSelectionTreeViewport", true);
    treeWidget->viewport()->installEventFilter(this);
    treeWidget->header()->setStretchLastSection(true);
    treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);

    const int addableRole = Qt::UserRole + 2;
    const int itemKindRole = Qt::UserRole + 3;
    const int expandableRole = Qt::UserRole + 4;

    const QColor disclosureColor = ThemeManager::instance()->currentTheme() == ThemeManager::Light
        ? QColor("#3a3a3c")
        : QColor("#d0d0d4");
    const QColor selectedDisclosureColor = QColor("#ffffff");

    auto createDisclosureIcon = [](const QColor& color, bool expanded) {
        QPixmap pixmap(14, 14);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);

        QPainterPath path;
        if (expanded) {
            path.moveTo(3.5, 5.0);
            path.lineTo(7.0, 9.0);
            path.lineTo(10.5, 5.0);
        } else {
            path.moveTo(5.0, 3.5);
            path.lineTo(9.0, 7.0);
            path.lineTo(5.0, 10.5);
        }
        painter.drawPath(path);
        painter.end();

        return QIcon(pixmap);
    };

    std::function<void(QTreeWidgetItem*)> updateProjectTreeIcons;
    auto refreshProjectTreeIcons = [&]() {
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            updateProjectTreeIcons(treeWidget->topLevelItem(i));
        }
    };

    updateProjectTreeIcons = [&](QTreeWidgetItem* item) {
        if (!item) {
            return;
        }

        const bool expandable = item->data(0, expandableRole).toBool();
        if (expandable) {
            const bool selected = treeWidget->currentItem() == item;
            item->setIcon(0, createDisclosureIcon(selected ? selectedDisclosureColor : disclosureColor,
                                                   item->isExpanded()));
        } else {
            item->setIcon(0, QIcon());
        }

        for (int i = 0; i < item->childCount(); ++i) {
            updateProjectTreeIcons(item->child(i));
        }
    };
    
    // Get known vaults from Obsidian config for status display
    QSet<QString> openVaults;
    QList<ObsidianVaultInfo> obsidianVaults = ObsidianConfigReader::instance().getObsidianVaults();
    for (const auto& info : obsidianVaults) {
        if (info.isOpen) {
            openVaults.insert(info.path);
        }
    }
    
    // Helper lambda to populate directory
    auto populateDirectory = [&](const QString& path, QTreeWidgetItem* parentItem) {
        QDir dir(path);
        if (!dir.exists()) return;
        
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);
        QStringList subdirs = dir.entryList();
        
        // Skip common non-interesting directories
        QStringList skipDirs = {".git", ".svn", ".hg", "node_modules", ".Trash", 
                                "Library", "Applications", "System"};
        
        for (const QString& subdir : subdirs) {
            if (skipDirs.contains(subdir)) continue;
            if (subdir.startsWith('.')) continue;  // Skip hidden directories except .obsidian
            
            QString fullPath = dir.filePath(subdir);
            
            QTreeWidgetItem* item;
            if (parentItem) {
                item = new QTreeWidgetItem(parentItem);
            } else {
                item = new QTreeWidgetItem(treeWidget);
            }
            
            item->setData(0, Qt::UserRole, fullPath);
            
            // Any directory can be added for monitoring; Obsidian only affects preview/open behavior.
            QDir subDir(fullPath);
            bool hasObsidian = subDir.exists(".obsidian");
            bool alreadyAdded = m_vaultModel->contains(fullPath);
            
            QString defaultName = m_vaultValidator->getVaultName(fullPath);

            item->setText(0, hasObsidian ? ("📁 " + subdir) : ("📂 " + subdir));
            item->setText(2, fullPath);
            item->setData(0, Qt::UserRole + 1, defaultName);
            item->setData(0, addableRole, !alreadyAdded);
            item->setData(0, itemKindRole, QStringLiteral("filesystem"));
            item->setData(0, expandableRole, true);

            if (alreadyAdded) {
                item->setText(1, "[Added]");
                item->setForeground(1, QColor("#86868b"));
            } else if (hasObsidian && openVaults.contains(fullPath)) {
                item->setText(1, "● Open");
                item->setForeground(1, QColor("#34c759"));
            } else if (hasObsidian) {
                item->setText(1, "Vault");
                item->setForeground(1, QColor("#007aff"));
            } else {
                item->setText(1, "Directory");
                item->setForeground(1, QColor("#86868b"));
            }

            // Add placeholder child to show expand arrow for further navigation
            QTreeWidgetItem* placeholder = new QTreeWidgetItem(item);
            placeholder->setText(0, tr("Loading..."));
            placeholder->setData(0, Qt::UserRole, "");
            placeholder->setData(0, addableRole, false);
            placeholder->setData(0, itemKindRole, QStringLiteral("placeholder"));
            placeholder->setData(0, expandableRole, false);
            if (!hasObsidian) {
                item->setForeground(0, QColor("#86868b"));
            }
            
            item->setToolTip(0, fullPath);
        }
    };
    
    // Add home directory as root
    QString homePath = QDir::homePath();
    QTreeWidgetItem* homeItem = new QTreeWidgetItem(treeWidget);
    homeItem->setText(0, "🏠 ~ (Home)");
    homeItem->setText(1, "");
    homeItem->setText(2, homePath);
    homeItem->setData(0, Qt::UserRole, homePath);
    homeItem->setData(0, addableRole, false);
    homeItem->setData(0, itemKindRole, QStringLiteral("filesystem_root"));
    homeItem->setData(0, expandableRole, true);
    homeItem->setForeground(0, QColor("#86868b"));
    homeItem->setForeground(2, QColor("#86868b"));
    
    // Add placeholder to show expand arrow
    QTreeWidgetItem* homePlaceholder = new QTreeWidgetItem(homeItem);
    homePlaceholder->setText(0, tr("Loading..."));
    homePlaceholder->setData(0, Qt::UserRole, "");
    homePlaceholder->setData(0, addableRole, false);
    homePlaceholder->setData(0, itemKindRole, QStringLiteral("placeholder"));
    homePlaceholder->setData(0, expandableRole, false);
    
    // Also add known vaults from Obsidian config as quick access
    QTreeWidgetItem* knownItem = new QTreeWidgetItem(treeWidget);
    knownItem->setText(0, "⭐ " + tr("Known Vaults (from Obsidian)"));
    knownItem->setText(1, "");
    knownItem->setText(2, "");
    knownItem->setData(0, Qt::UserRole, "");
    knownItem->setData(0, addableRole, false);
    knownItem->setData(0, itemKindRole, QStringLiteral("known_group"));
    knownItem->setData(0, expandableRole, true);
    knownItem->setForeground(0, QColor("#007aff"));
    QFont knownFont = knownItem->font(0);
    knownFont.setBold(true);
    knownItem->setFont(0, knownFont);
    
    // Add known vaults
    for (const auto& info : obsidianVaults) {
        if (!info.exists || !info.isValid) continue;
        if (m_vaultModel->contains(info.path)) continue;
        
        QTreeWidgetItem* vaultItem = new QTreeWidgetItem(knownItem);
        
        // Column 0: Name
        vaultItem->setText(0, "📁 " + info.name);
        
        // Column 1: Status with color
        if (info.isOpen) {
            vaultItem->setText(1, "● Open");
            vaultItem->setForeground(1, QColor("#34c759"));
        } else {
            vaultItem->setText(1, "Vault");
            vaultItem->setForeground(1, QColor("#007aff"));
        }
        
        // Column 2: Path
        vaultItem->setText(2, info.path);
        
        vaultItem->setData(0, Qt::UserRole, info.path);
        vaultItem->setData(0, Qt::UserRole + 1, info.name);
        vaultItem->setData(0, addableRole, true);
        vaultItem->setData(0, itemKindRole, QStringLiteral("known_vault"));
        vaultItem->setData(0, expandableRole, false);
        
        vaultItem->setToolTip(0, info.path);
    }
    
    // If no known vaults, remove the section
    if (knownItem->childCount() == 0) {
        delete knownItem;
    }
    
    layout->addWidget(treeWidget);
    
    // Info label
    QLabel* infoLabel = new QLabel(tr("📁 = Obsidian vault  |  📂 = Directory for monitoring  |  ● Open = Currently open in Obsidian"));
    infoLabel->setStyleSheet("color: #86868b; font-size: 12px;");
    layout->addWidget(infoLabel);
    
    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton* addBtn = new QPushButton(tr("Add"));
    addBtn->setDefault(true);
    addBtn->setFixedWidth(80);
    addBtn->setEnabled(false);
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setFixedWidth(80);
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
    
    // Handle item expansion - lazy load subdirectories
    connect(treeWidget, &QTreeWidget::itemExpanded, [&](QTreeWidgetItem* item) {
        // Check if this is a placeholder that needs loading
        if (item->childCount() == 1) {
            QTreeWidgetItem* child = item->child(0);
            if (child && child->text(0) == tr("Loading...")) {
                // Remove placeholder and load actual content
                delete child;
                QString path = item->data(0, Qt::UserRole).toString();
                if (!path.isEmpty()) {
                    populateDirectory(path, item);
                }
                item->setData(0, expandableRole, item->childCount() > 0);
            }
        }
        refreshProjectTreeIcons();
    });
    connect(treeWidget, &QTreeWidget::itemCollapsed, [&](QTreeWidgetItem*) {
        refreshProjectTreeIcons();
    });
    connect(treeWidget, &QTreeWidget::itemSelectionChanged, [&]() {
        QTreeWidgetItem* selected = treeWidget->currentItem();
        addBtn->setEnabled(selected && selected->data(0, addableRole).toBool());
        refreshProjectTreeIcons();
    });
    
    // Connect buttons
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(addBtn, &QPushButton::clicked, [&]() {
        QTreeWidgetItem* selected = treeWidget->currentItem();
        if (selected && selected->data(0, addableRole).toBool()) {
            dialog.accept();
        } else {
            QMessageBox::information(&dialog, tr("Select Directory"),
                tr("Please select a project directory that is not already added."));
        }
    });
    connect(treeWidget, &QTreeWidget::itemDoubleClicked, [&](QTreeWidgetItem* item, int) {
        if (!item) {
            return;
        }

        if (item->data(0, addableRole).toBool()) {
            dialog.accept();
        }
    });
    
    // Expand home directory by default
    treeWidget->expandItem(homeItem);
    refreshProjectTreeIcons();
    
    // Apply theme style to dialog
    dialog.setStyleSheet(ThemeManager::instance()->currentStyle());
    
    if (dialog.exec() == QDialog::Accepted) {
        QTreeWidgetItem* selected = treeWidget->currentItem();
        if (selected) {
            QString path = selected->data(0, Qt::UserRole).toString();
            QString defaultName = selected->data(0, Qt::UserRole + 1).toString();
            
            if (defaultName.isEmpty()) {
                defaultName = m_vaultValidator->getVaultName(path);
            }
            
            QString name = QInputDialog::getText(this, tr("Project Name"), tr("Enter project name:"),
                                                  QLineEdit::Normal, defaultName);
            if (name.isEmpty()) return;
            
            Vault vault;
            vault.name = name;
            vault.path = path;
            vault.addedAt = QDateTime::currentDateTime();
            
            m_vaultModel->addVault(vault);
            m_vaultModel->save(m_configPath);
        }
    }
}

void MainWindow::onRemoveVault()
{
    QListWidgetItem *item = m_vaultList->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("Warning"), tr("No vault selected."));
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    m_vaultModel->removeVault(path);
    m_vaultModel->save(m_configPath);
    setOverviewEmptyState(true);
}

void MainWindow::onRenameVault()
{
    QListWidgetItem *item = m_vaultList->currentItem();
    if (!item) return;

    // Store the path before editing
    m_editingVaultPath = item->data(Qt::UserRole).toString();
    
    // Enter edit mode directly
    m_vaultList->editItem(item);
}

void MainWindow::onOpenInObsidian()
{
    QListWidgetItem *item = m_vaultList->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("Warning"), tr("No vault selected."));
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    if (resolveObsidianOpenPath(path).isEmpty()) {
        QMessageBox::information(this, tr("Unavailable"),
                                 tr("This project directory has no Obsidian configuration."));
        return;
    }
    openVaultInObsidian(path);
}

void MainWindow::onVaultDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    QString path = item->data(Qt::UserRole).toString();
    if (resolveObsidianOpenPath(path).isEmpty()) {
        return;
    }
    openVaultInObsidian(path);
}

void MainWindow::onVaultContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_vaultList->itemAt(pos);
    if (!item) return;

    // Store path for potential rename
    m_editingVaultPath = item->data(Qt::UserRole).toString();

    QMenu contextMenu(tr("Context Menu"), this);
    
    QAction *renameAction = contextMenu.addAction(tr("Rename"));
    renameAction->setShortcut(QKeySequence("F2"));
    
    QAction *openAction = nullptr;
    if (!resolveObsidianOpenPath(item->data(Qt::UserRole).toString()).isEmpty()) {
        openAction = contextMenu.addAction(tr("Open in Obsidian"));
        openAction->setShortcut(QKeySequence("Ctrl+O"));
    }
    
    contextMenu.addSeparator();
    
    QAction *removeAction = contextMenu.addAction(tr("Remove"));
    
    QAction *selectedAction = contextMenu.exec(m_vaultList->mapToGlobal(pos));
    
    if (selectedAction == renameAction) {
        m_editingVaultPath = item->data(Qt::UserRole).toString();
        m_vaultList->editItem(item);
    } else if (openAction && selectedAction == openAction) {
        QString path = item->data(Qt::UserRole).toString();
        openVaultInObsidian(path);
    } else if (selectedAction == removeAction) {
        QString path = item->data(Qt::UserRole).toString();
        m_vaultModel->removeVault(path);
        m_vaultModel->save(m_configPath);
        setOverviewEmptyState(true);
    }
}

void MainWindow::onVaultItemChanged(QListWidgetItem *item)
{
    if (m_editingVaultPath.isEmpty()) return;
    
    QString newName = item->text().trimmed();
    if (newName.isEmpty()) {
        // Restore old name
        int idx = m_vaultModel->indexOf(m_editingVaultPath);
        if (idx >= 0) {
            Vault vault = m_vaultModel->vaultAt(idx);
            item->setText(vault.name);
        }
        m_editingVaultPath.clear();
        return;
    }
    
    // Update the vault model
    int idx = m_vaultModel->indexOf(m_editingVaultPath);
    if (idx >= 0) {
        Vault vault = m_vaultModel->vaultAt(idx);
        if (vault.name != newName) {
            vault.name = newName;
            m_vaultModel->updateVault(m_editingVaultPath, vault);
            m_vaultModel->save(m_configPath);
            
            // Update preview if this vault is currently selected
            if (m_currentPreviewPath == m_editingVaultPath) {
                m_previewTitle->setText(newName);
            }
        }
    }
    
    m_editingVaultPath.clear();
}

QString MainWindow::resolveObsidianOpenPath(const QString& vaultPath) const
{
    // Open priority:
    // 1) If .r2mo/.obsidian exists -> open .r2mo
    // 2) Otherwise, if .obsidian exists in project root -> open project root

    QDir dir(vaultPath);
    QString openPath;
    
    // Case A: input path itself is .r2mo directory
    if (vaultPath.endsWith("/.r2mo") || vaultPath.endsWith("\\.r2mo")) {
        QDir r2moDir(vaultPath);
        if (r2moDir.exists(".obsidian")) {
            openPath = vaultPath;
        }
    } else {
        // First priority: .r2mo/.obsidian
        QString r2moPath = dir.filePath(".r2mo");
        QDir r2moDir(r2moPath);
        if (r2moDir.exists(".obsidian")) {
            openPath = r2moPath;
        } else if (dir.exists(".obsidian")) {
            // Fallback: project root .obsidian
            openPath = vaultPath;
        }
    }

    return openPath;
}

void MainWindow::openVaultInObsidian(const QString& vaultPath)
{
    const QString openPath = resolveObsidianOpenPath(vaultPath);
    if (openPath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), 
            tr("Not a valid Obsidian vault.\n\nPath: %1\n\nRequired: .r2mo/.obsidian or .obsidian directory").arg(vaultPath));
        return;
    }
    
    // Ensure vault is registered in Obsidian config
    ObsidianVaultInfo vaultInfo = ObsidianConfigReader::instance().getVaultByPath(openPath);
    
    if (vaultInfo.id.isEmpty()) {
        QString registeredId = ObsidianConfigReader::instance().registerVault(openPath);
        if (registeredId.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), 
                tr("Failed to register vault to Obsidian configuration."));
            return;
        }
    }
    
    // Open vault via URL scheme
    QString encodedPath = QUrl::toPercentEncoding(openPath);
    QString obsidianUrl = QString("obsidian://open?path=%1").arg(encodedPath);
    
    QProcess process;
    process.startDetached("open", QStringList{obsidianUrl});
    
    statusBar()->showMessage(tr("Opening vault: %1").arg(openPath), 3000);
}

void MainWindow::onSettings()
{
    QString path = QInputDialog::getText(this, tr("Obsidian App Path"),
                                         tr("Enter Obsidian.app path:"),
                                         QLineEdit::Normal,
                                         m_settingsManager->obsidianAppPath());
    if (!path.isEmpty()) {
        m_settingsManager->setObsidianAppPath(path);
    }
}

void MainWindow::refreshSpecialMonitorAsync()
{
    if (m_specialMonitorRefreshing) {
        return;
    }

    const QList<SpecialMonitorSource> sources = m_settingsManager->specialMonitorSources();
    if (sources.isEmpty()) {
        updateSpecialMonitorTable({});
        return;
    }

    m_specialMonitorRefreshing = true;
    m_specialMonitorStatusLabel->setText(tr("Refreshing..."));
    QFuture<QList<SpecialMonitorSnapshot>> future = QtConcurrent::run([sources]() {
        SpecialMonitorFetcher fetcher;
        return fetcher.fetchSnapshots(sources);
    });
    m_specialMonitorScanWatcher->setFuture(future);
}

void MainWindow::updateSpecialMonitorTable(const QList<SpecialMonitorSnapshot>& snapshots)
{
    if (!m_specialMonitorTable) {
        return;
    }

    const QList<SpecialMonitorSource> sources = m_settingsManager->specialMonitorSources();
    m_specialMonitorTable->setRowCount(sources.size());
    auto makeItem = [](const QString& text, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(alignment);
        return item;
    };

    for (int row = 0; row < sources.size(); ++row) {
        SpecialMonitorSnapshot snapshot;
        if (row < snapshots.size()) {
            snapshot = snapshots.at(row);
        } else {
            snapshot.source = sources.at(row);
            snapshot.errorMessage = tr("Not loaded");
        }
        const QString provider = snapshot.source.providerName.isEmpty()
            ? SpecialMonitorFetcher::defaultProviderName(snapshot.source.baseUrl)
            : snapshot.source.providerName;
        const QString tokenLabel = snapshot.source.tokenKey;
        const QString typeLabel = snapshot.packageType.isEmpty() ? tr("Unknown") : snapshot.packageType;
        const QString accountLabel = snapshot.accountName.isEmpty() ? tr("Unknown") : snapshot.accountName;

        m_specialMonitorTable->setItem(row, 0, makeItem(provider));
        QTableWidgetItem *tokenItem = makeItem(QString());
        tokenItem->setToolTip(QString());
        m_specialMonitorTable->setItem(row, 1, tokenItem);
        QWidget *tokenWidget = new QWidget(m_specialMonitorTable);
        QHBoxLayout *tokenLayout = new QHBoxLayout(tokenWidget);
        tokenLayout->setContentsMargins(8, 0, 8, 0);
        tokenLayout->setSpacing(6);
        QToolButton *copyBtn = new QToolButton(tokenWidget);
        copyBtn->setText(tr("复制"));
        copyBtn->setAutoRaise(true);
        copyBtn->setCursor(Qt::PointingHandCursor);
        copyBtn->setToolTip(QString());
        copyBtn->setFixedWidth(34);
        QLabel *tokenLabelWidget = new QLabel(tokenWidget);
        tokenLabelWidget->setObjectName("specialMonitorTokenLabel");
        tokenLabelWidget->setProperty("fullToken", tokenLabel);
        tokenLabelWidget->setToolTip(QString());
        tokenLabelWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        tokenLabelWidget->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        tokenLayout->addWidget(copyBtn, 0);
        tokenLayout->addWidget(tokenLabelWidget, 1);
        connect(copyBtn, &QToolButton::clicked, this, [this, tokenLabel]() {
            if (QClipboard *clipboard = QApplication::clipboard()) {
                clipboard->setText(tokenLabel, QClipboard::Clipboard);
#if defined(Q_OS_LINUX)
                clipboard->setText(tokenLabel, QClipboard::Selection);
#endif
                statusBar()->showMessage(tr("Token copied"), 2000);
            }
        });
        m_specialMonitorTable->setCellWidget(row, 1, tokenWidget);
        m_specialMonitorTable->setItem(row, 2, makeItem(accountLabel));
        m_specialMonitorTable->setItem(row, 3, makeItem(typeLabel));
        m_specialMonitorTable->setItem(row, 4, makeItem(QString::number(snapshot.todayAddedQuota), Qt::AlignCenter));
        m_specialMonitorTable->setItem(row, 5, makeItem(QString::number(snapshot.todayUsedQuota), Qt::AlignCenter));
        m_specialMonitorTable->setItem(row, 6, makeItem(QString::number(snapshot.remainQuota), Qt::AlignCenter));
        m_specialMonitorTable->setItem(row, 7, makeItem(QString::number(snapshot.todayUsageCount), Qt::AlignCenter));
        m_specialMonitorTable->setItem(row, 8, makeItem(QString::number(snapshot.todayOpusUsage), Qt::AlignCenter));
        m_specialMonitorTable->setItem(row, 9, makeItem(snapshot.updatedAt, Qt::AlignCenter));
    }

    m_specialMonitorStatusLabel->setText(
        sources.isEmpty() ? tr("No token sources") : tr("%1 token(s)").arg(sources.size()));

    updateSpecialMonitorTableColumns();
}

void MainWindow::updateSpecialMonitorTableColumns()
{
    if (!m_specialMonitorTable || m_specialMonitorTable->columnCount() < 10) {
        return;
    }

    const int viewportWidth = m_specialMonitorTable->viewport()->width();
    if (viewportWidth <= 0) {
        return;
    }

    int providerWidth = qBound(120, viewportWidth / 10, 180);
    int tokenWidth = qBound(180, viewportWidth / 8, 280);
    int accountWidth = qBound(150, viewportWidth / 9, 220);
    int typeWidth = qBound(110, viewportWidth / 12, 150);
    int addedWidth = qBound(110, viewportWidth / 13, 150);
    int usedWidth = qBound(110, viewportWidth / 13, 150);
    int remainWidth = qBound(110, viewportWidth / 13, 150);
    int usageWidth = qBound(110, viewportWidth / 13, 150);
    int opusWidth = qBound(128, viewportWidth / 12, 176);
    int updatedWidth = qBound(150, viewportWidth / 9, 220);
    const int chromeWidth = 8;

    auto totalWidth = [&]() {
        return providerWidth + tokenWidth + accountWidth + typeWidth + addedWidth +
            usedWidth + remainWidth + usageWidth + opusWidth + updatedWidth + chromeWidth;
    };

    const int extraWidth = viewportWidth - totalWidth();
    if (extraWidth > 0) {
        tokenWidth += extraWidth / 3;
        updatedWidth += extraWidth / 4;
        accountWidth += extraWidth / 6;
        providerWidth += extraWidth - (extraWidth / 3) - (extraWidth / 4) - (extraWidth / 6);
    } else if (extraWidth < 0) {
        int deficit = -extraWidth;
        auto shrink = [&deficit](int &value, int minValue) {
            if (deficit <= 0 || value <= minValue) {
                return;
            }
            const int delta = qMin(deficit, value - minValue);
            value -= delta;
            deficit -= delta;
        };

        shrink(tokenWidth, 160);
        shrink(updatedWidth, 136);
        shrink(accountWidth, 132);
        shrink(providerWidth, 104);
        shrink(typeWidth, 96);
        shrink(addedWidth, 96);
        shrink(usedWidth, 96);
        shrink(remainWidth, 96);
        shrink(usageWidth, 96);
        shrink(opusWidth, 120);

        if (deficit > 0) {
            tokenWidth = qMax(128, tokenWidth - deficit);
        }
    }

    m_specialMonitorTable->setColumnWidth(0, providerWidth);
    m_specialMonitorTable->setColumnWidth(1, tokenWidth);
    m_specialMonitorTable->setColumnWidth(2, accountWidth);
    m_specialMonitorTable->setColumnWidth(3, typeWidth);
    m_specialMonitorTable->setColumnWidth(4, addedWidth);
    m_specialMonitorTable->setColumnWidth(5, usedWidth);
    m_specialMonitorTable->setColumnWidth(6, remainWidth);
    m_specialMonitorTable->setColumnWidth(7, usageWidth);
    m_specialMonitorTable->setColumnWidth(8, opusWidth);
    m_specialMonitorTable->setColumnWidth(9, updatedWidth);

    for (int row = 0; row < m_specialMonitorTable->rowCount(); ++row) {
        QWidget *tokenWidget = m_specialMonitorTable->cellWidget(row, 1);
        if (!tokenWidget) {
            continue;
        }
        QLabel *tokenLabelWidget = tokenWidget->findChild<QLabel*>("specialMonitorTokenLabel");
        if (!tokenLabelWidget) {
            continue;
        }
        const QString fullToken = tokenLabelWidget->property("fullToken").toString();
        const int labelWidth = qMax(48, m_specialMonitorTable->columnWidth(1) - 58);
        const QString elided = tokenLabelWidget->fontMetrics().elidedText(fullToken, Qt::ElideRight, labelWidth);
        tokenLabelWidget->setText(elided);
    }
}

QWidget* MainWindow::buildSpecialMonitorPanel()
{
    QWidget *panel = new QWidget();
    panel->setStyleSheet("QWidget { background: #fafafa; border-top: 1px solid #e0e0e0; }");
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel(tr("🔑 ⚙️ Special Billing"));
    title->setStyleSheet("QLabel { font-size: 15px; font-weight: 600; color: #333; }");
    headerLayout->addWidget(title);

    m_specialMonitorStatusLabel = new QLabel(tr("Idle"));
    m_specialMonitorStatusLabel->setStyleSheet("QLabel { color: #86868b; font-size: 14px; }");
    headerLayout->addWidget(m_specialMonitorStatusLabel);
    headerLayout->addStretch();

    QPushButton *addBtn = new QPushButton(tr("Add"));
    QPushButton *editBtn = new QPushButton(tr("Edit"));
    QPushButton *removeBtn = new QPushButton(tr("Remove"));
    QPushButton *refreshBtn = new QPushButton(tr("Refresh"));
    const QString actionButtonStyle =
        "QPushButton { background: #f5f5f7; color: #4a4a4a; font-size: 11px; border: 1px solid #d7d7dc; border-radius: 4px; padding: 2px 10px; min-width: 80px; min-height: 24px; }"
        "QPushButton:hover { background: #ececf1; color: #1d1d1f; }";
    const QString dangerButtonStyle =
        "QPushButton { background: #fff1f0; color: #d93025; font-size: 11px; border: 1px solid #f1b7b3; border-radius: 4px; padding: 2px 10px; min-width: 80px; min-height: 24px; }"
        "QPushButton:hover { background: #ffe3e0; color: #b3261e; }";
    addBtn->setStyleSheet(actionButtonStyle);
    editBtn->setStyleSheet(actionButtonStyle);
    removeBtn->setStyleSheet(dangerButtonStyle);
    refreshBtn->setStyleSheet(actionButtonStyle);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addSpecialMonitorSource);
    connect(editBtn, &QPushButton::clicked, this, &MainWindow::editSpecialMonitorSource);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::removeSpecialMonitorSource);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshSpecialMonitorAsync);
    headerLayout->addWidget(addBtn);
    headerLayout->addWidget(editBtn);
    headerLayout->addWidget(removeBtn);
    headerLayout->addWidget(refreshBtn);
    layout->addLayout(headerLayout);

    m_specialMonitorTable = new QTableWidget(0, 10, panel);
    m_specialMonitorTable->setHorizontalHeaderLabels(QStringList()
        << tr("Provider")
        << tr("Token")
        << tr("🔑 Account")
        << tr("⚙️ Type")
        << tr("➕ Today Added")
        << tr("🔥 Today Used")
        << tr("🎯 Remain")
        << tr("📈 Usage Count")
        << tr("💎 Opus Count")
        << tr("🕒 Updated"));
    m_specialMonitorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_specialMonitorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_specialMonitorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_specialMonitorTable->setAlternatingRowColors(true);
    m_specialMonitorTable->setShowGrid(false);
    m_specialMonitorTable->setFrameShape(QFrame::NoFrame);
    m_specialMonitorTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_specialMonitorTable->setTextElideMode(Qt::ElideRight);
    m_specialMonitorTable->horizontalHeader()->setStretchLastSection(false);
    m_specialMonitorTable->horizontalHeader()->setSectionsMovable(false);
    m_specialMonitorTable->horizontalHeader()->setSectionsClickable(true);
    for (int col = 0; col < m_specialMonitorTable->columnCount(); ++col) {
        m_specialMonitorTable->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Interactive);
    }
    m_specialMonitorTable->verticalHeader()->setVisible(false);
    m_specialMonitorTable->verticalHeader()->setDefaultSectionSize(38);
    m_specialMonitorTable->setMinimumHeight(220);
    m_specialMonitorTable->horizontalHeader()->setStretchLastSection(false);
    m_specialMonitorTable->setStyleSheet(
        "QTableWidget { background: white; border: none; gridline-color: transparent; font-size: 14px; outline: none; }"
        "QTableWidget::item { padding: 8px 10px; border-bottom: 1px solid #f5f5f7; }"
        "QTableWidget::item:selected { background: #e8f4fd; color: #1d1d1f; }"
        "QHeaderView::section { background: #f5f5f7; color: #86868b; font-size: 14px; font-weight: 600; min-height: 36px; padding: 6px 8px; border: none; border-bottom: 1px solid #e0e0e0; }");
    m_specialMonitorTable->viewport()->installEventFilter(this);
    QTimer::singleShot(0, m_specialMonitorTable, [this]() {
        if (!m_specialMonitorTable) {
            return;
        }
        updateSpecialMonitorTableColumns();
        const QByteArray headerState = m_settingsManager->specialMonitorHeaderState();
        if (!headerState.isEmpty()) {
            m_specialMonitorTable->horizontalHeader()->restoreState(headerState);
        }
        updateSpecialMonitorTableColumns();
    });
    connect(m_specialMonitorTable->horizontalHeader(), &QHeaderView::sectionResized, this, [this]() {
        if (!m_specialMonitorTable) {
            return;
        }
        m_settingsManager->setSpecialMonitorHeaderState(m_specialMonitorTable->horizontalHeader()->saveState());
        m_settingsManager->sync();
    });
    layout->addWidget(m_specialMonitorTable, 1);

    updateSpecialMonitorTable({});

    return panel;
}

void MainWindow::updateSpecialMonitorPanelSizing()
{
    if (!m_monitorVerticalSplitter || !m_specialMonitorPanel) {
        return;
    }

    const int totalHeight = qMax(420, m_monitorVerticalSplitter->size().height());
    const int minPanelHeight = qMax(180, totalHeight / 5);
    const int preferredPanelHeight = qMax(minPanelHeight, (totalHeight * 2) / 5);
    const int maxPanelHeight = qMax(preferredPanelHeight, totalHeight / 2);
    const QList<int> currentSizes = m_monitorVerticalSplitter->sizes();
    const int currentPanelHeight = currentSizes.size() > 1 ? currentSizes.at(1) : 0;
    const int targetPanelHeight = (!m_specialMonitorPanelUserResized || currentPanelHeight <= 0)
        ? preferredPanelHeight
        : qBound(minPanelHeight, currentPanelHeight, maxPanelHeight);
    const int topHeight = qMax(120, totalHeight - targetPanelHeight);

    m_specialMonitorPanel->setMinimumHeight(minPanelHeight);
    m_specialMonitorPanel->setMaximumHeight(maxPanelHeight);
    m_monitorVerticalSplitter->setSizes(QList<int>() << topHeight << targetPanelHeight);
    m_monitorVerticalSplitter->setCollapsible(0, false);
    m_monitorVerticalSplitter->setCollapsible(1, false);
}

void MainWindow::addSpecialMonitorSource()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Token Source"));
    dialog.resize(720, 200);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(12);

    QComboBox *providerCombo = new QComboBox(&dialog);
    const QList<SpecialMonitorProviderOption> providers = specialMonitorProviders();
    for (const SpecialMonitorProviderOption& provider : providers) {
        providerCombo->addItem(provider.label, provider.baseUrl);
    }
    providerCombo->setMinimumWidth(320);

    QLineEdit *tokenEdit = new QLineEdit(&dialog);
    tokenEdit->setPlaceholderText(tr("Enter token key"));
    tokenEdit->setMinimumWidth(520);
    tokenEdit->setClearButtonEnabled(true);
    formLayout->addRow(tr("Provider"), providerCombo);
    formLayout->addRow(tr("Token Key"), tokenEdit);
    dialogLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString baseUrl = providerCombo->currentData().toString();
    const QString tokenKey = tokenEdit->text().trimmed();
    if (tokenKey.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Token key cannot be empty."));
        return;
    }

    QList<SpecialMonitorSource> sources = m_settingsManager->specialMonitorSources();
    const QString newKey = specialMonitorSourceKey(baseUrl, tokenKey);
    for (const SpecialMonitorSource& existing : sources) {
        if (specialMonitorSourceKey(existing.baseUrl, existing.tokenKey) == newKey) {
            QMessageBox::warning(this, tr("Duplicate Token Source"),
                                 tr("This provider and token already exist."));
            return;
        }
    }

    SpecialMonitorSource source;
    source.baseUrl = baseUrl;
    source.tokenKey = tokenKey;
    source.providerName = providerCombo->currentText();
    sources.append(source);
    m_settingsManager->setSpecialMonitorSources(sources);
    refreshSpecialMonitorAsync();
}

void MainWindow::editSpecialMonitorSource()
{
    if (!m_specialMonitorTable || m_specialMonitorTable->currentRow() < 0) {
        return;
    }
    const int row = m_specialMonitorTable->currentRow();
    QList<SpecialMonitorSource> sources = m_settingsManager->specialMonitorSources();
    if (row >= sources.size()) {
        return;
    }

    SpecialMonitorSource source = sources.at(row);
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Token Source"));
    dialog.resize(720, 200);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(12);

    QComboBox *providerCombo = new QComboBox(&dialog);
    const QList<SpecialMonitorProviderOption> providers = specialMonitorProviders();
    int selectedIndex = 0;
    for (int i = 0; i < providers.size(); ++i) {
        providerCombo->addItem(providers.at(i).label, providers.at(i).baseUrl);
        if (SpecialMonitorFetcher::normalizeBaseUrl(providers.at(i).baseUrl) ==
            SpecialMonitorFetcher::normalizeBaseUrl(source.baseUrl)) {
            selectedIndex = i;
        }
    }
    providerCombo->setCurrentIndex(selectedIndex);
    providerCombo->setMinimumWidth(320);

    QLineEdit *tokenEdit = new QLineEdit(source.tokenKey, &dialog);
    tokenEdit->setPlaceholderText(tr("Enter token key"));
    tokenEdit->setMinimumWidth(520);
    tokenEdit->setClearButtonEnabled(true);
    formLayout->addRow(tr("Provider"), providerCombo);
    formLayout->addRow(tr("Token Key"), tokenEdit);
    dialogLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString baseUrl = providerCombo->currentData().toString();
    const QString tokenKey = tokenEdit->text().trimmed();
    if (tokenKey.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Token key cannot be empty."));
        return;
    }

    const QString updatedKey = specialMonitorSourceKey(baseUrl, tokenKey);
    for (int i = 0; i < sources.size(); ++i) {
        if (i == row) {
            continue;
        }
        if (specialMonitorSourceKey(sources.at(i).baseUrl, sources.at(i).tokenKey) == updatedKey) {
            QMessageBox::warning(this, tr("Duplicate Token Source"),
                                 tr("This provider and token already exist."));
            return;
        }
    }

    source.baseUrl = baseUrl;
    source.tokenKey = tokenKey;
    source.providerName = providerCombo->currentText();
    sources[row] = source;
    m_settingsManager->setSpecialMonitorSources(sources);
    refreshSpecialMonitorAsync();
}

void MainWindow::removeSpecialMonitorSource()
{
    if (!m_specialMonitorTable || m_specialMonitorTable->currentRow() < 0) {
        return;
    }
    const int row = m_specialMonitorTable->currentRow();
    QList<SpecialMonitorSource> sources = m_settingsManager->specialMonitorSources();
    if (row >= sources.size()) {
        return;
    }
    sources.removeAt(row);
    m_settingsManager->setSpecialMonitorSources(sources);
    refreshSpecialMonitorAsync();
}

void MainWindow::updatePreviewPane(const QString& name, const QString& path)
{
    m_currentPreviewPath = path;
    const bool hasObsidian = !resolveObsidianOpenPath(path).isEmpty();
    
    // Update title and show buttons
    m_previewTitle->setText(name);
    m_previewTitle->show();
    m_previewEditBtn->show();
    m_previewOpenBtn->setVisible(hasObsidian);
    m_previewTitleEdit->hide();
    
    // Clear previous content
    m_graphScene->clear();
    m_taskTree->clear();
    if (path.trimmed().isEmpty()) {
        m_overviewEmptyLabel->setText(tr("Select a vault to view details..."));
        setOverviewEmptyState(true);
        buildAIToolsTab(QString());
        return;
    }

    if (!hasObsidian) {
        m_overviewEmptyLabel->setText(tr("No Obsidian configuration found for this project."));
        setOverviewEmptyState(true);
        m_r2moStatsCard->setVisible(false);
        m_graphScene->clear();
        m_taskTree->clear();
        buildAIToolsTab(QString());
        return;
    }

    setOverviewEmptyState(false);
    m_overviewEmptyLabel->setText(tr("Select a vault to view details..."));
    m_overviewPathLabel->setText(path);
    clearOverviewGrid(m_vaultStatsGrid);
    clearOverviewGrid(m_r2moStatsGrid);

    QDir vaultDir(path);
    if (vaultDir.exists()) {
        // Show hidden files in count (including .obsidian, .r2mo, etc.)
        vaultDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
        QStringList allEntries = vaultDir.entryList();
        
        int files = 0;
        int folders = 0;
        int hiddenItems = 0;
        
        for (const QString& entry : allEntries) {
            QFileInfo fi(path + "/" + entry);
            if (fi.isHidden()) {
                hiddenItems++;
            }
            if (fi.isFile()) {
                files++;
            } else if (fi.isDir()) {
                folders++;
            }
        }

        int vaultRow = 0;
        addOverviewRow(m_vaultStatsGrid, vaultRow++, tr("Files"), QString::number(files));
        addOverviewRow(m_vaultStatsGrid, vaultRow++, tr("Folders"), QString::number(folders));
        if (hiddenItems > 0) {
            addOverviewRow(m_vaultStatsGrid, vaultRow++, tr("Hidden Items"), QString::number(hiddenItems), QString(), tr("(.obsidian, .r2mo)"));
        }
        if (m_vaultValidator->hasObsidianConfig(path)) {
            addOverviewRow(m_vaultStatsGrid, vaultRow++, tr("Obsidian"), tr("✓ Valid Vault"), "#34c759");
        }
        
        // Check for .r2mo folder and show detailed info
        if (m_vaultValidator->hasR2moConfig(path)) {
            // Scan for R2MO projects
            R2moScanner scanner;
            QList<R2moSubProject> projects = scanner.scanVault(path);
            
            if (!projects.isEmpty()) {
                // Calculate totals
                int totalQueue = 0;
                int totalHistorical = 0;
                
                for (const R2moSubProject& proj : projects) {
                    totalQueue += proj.taskQueueCount;
                    totalHistorical += proj.historicalTaskCount;
                }
                
                int r2moRow = 0;
                addOverviewRow(m_r2moStatsGrid, r2moRow++, tr("Configuration"), tr("✓ Detected"), "#007aff");
                addOverviewRow(m_r2moStatsGrid, r2moRow++, tr("Total Projects"), QString::number(projects.size()));
                addOverviewRow(m_r2moStatsGrid, r2moRow++, tr("Task Queue"), QString("%1 %2").arg(totalQueue).arg(tr("pending")), "#ff9500");
                addOverviewRow(m_r2moStatsGrid, r2moRow++, tr("Historical Tasks"), QString("%1 %2").arg(totalHistorical).arg(tr("completed")), "#34c759");
                
                // Draw directed graph for project relationships (in Graph tab)
                drawProjectGraph(projects);
                
                // Build expandable task tree (in Tasks tab)
                buildTaskTree(projects);
            }
            m_r2moStatsCard->setVisible(!projects.isEmpty());
        } else {
            m_r2moStatsCard->setVisible(false);
        }
        
        // Build AI Tools tab
        buildAIToolsTab(path);
    } else {
        addOverviewRow(m_vaultStatsGrid, 0, tr("Warning"), tr("Path does not exist"), "#ff3b30");
        m_r2moStatsCard->setVisible(false);
        buildAIToolsTab(QString());
    }
}

void MainWindow::setOverviewEmptyState(bool empty)
{
    m_overviewEmptyLabel->setVisible(empty);
    m_overviewContent->setVisible(!empty);
    if (empty) {
        clearOverviewGrid(m_vaultStatsGrid);
        clearOverviewGrid(m_r2moStatsGrid);
    }
}

void MainWindow::clearOverviewGrid(QGridLayout *layout)
{
    if (!layout) return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
}

void MainWindow::addOverviewRow(QGridLayout *layout, int row, const QString& label, const QString& value,
                                const QString& valueColor, const QString& suffixHtml)
{
    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("QLabel { color: #86868b; font-size: 14px; padding: 14px 20px; border-bottom: 1px solid #e8e8ed; }");
    labelWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QString valueText = value;
    if (!suffixHtml.isEmpty()) {
        valueText += QString(" <span style='color: #86868b;'>%1</span>").arg(suffixHtml.toHtmlEscaped());
    }
    QLabel *valueWidget = new QLabel(valueText);
    valueWidget->setTextFormat(Qt::RichText);
    valueWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QString color = valueColor.isEmpty() ? QString("#1d1d1f") : valueColor;
    valueWidget->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; font-weight: 500; padding: 14px 20px; border-bottom: 1px solid #e8e8ed; }").arg(color));

    layout->addWidget(labelWidget, row, 0);
    layout->addWidget(valueWidget, row, 1);
}

void MainWindow::drawProjectGraph(const QList<R2moSubProject>& projects)
{
    m_graphScene->clear();
    
    // Determine colors based on current theme
    ThemeManager::Theme currentTheme = ThemeManager::instance()->currentTheme();
    
    // Colors for theme
    QColor textColor = (currentTheme == ThemeManager::Light) ? QColor("#333333") : QColor("#f5f5f7");
    QColor rulerBgColor = (currentTheme == ThemeManager::Light) ? QColor("#f5f5f7") : QColor("#3a3a3c");
    QColor rulerTextColor = (currentTheme == ThemeManager::Light) ? QColor("#86868b") : QColor("#98989d");
    QColor gridColor = (currentTheme == ThemeManager::Light) ? QColor("#e8e8ed") : QColor("#48484a");
    QColor parentBgColor = (currentTheme == ThemeManager::Light) ? QColor("#e8f4fd") : QColor("#1a3a5c");
    QColor childBgColor = (currentTheme == ThemeManager::Light) ? QColor("#e8f8e8") : QColor("#1a4a2c");
    QColor borderColor = QColor("#007aff");
    QColor arrowColor = QColor("#007aff");
    
    // Find parent project
    const R2moSubProject* parent = nullptr;
    QList<const R2moSubProject*> children;
    
    for (const R2moSubProject& proj : projects) {
        if (proj.isParent) {
            parent = &proj;
        } else {
            children.append(&proj);
        }
    }
    
    if (!parent) return;
    
    // Use a large infinite-style scene and draw grid across it
    QSize viewSize = m_graphView->viewport()->size();
    int viewWidth = qMax(viewSize.width(), 2400);
    int viewHeight = qMax(viewSize.height(), 1800);
    
    // Full-page grid background - starts from (0,0) to fill entire view
    int majorTickInterval = 100;
    int minorTickInterval = 20;
    QPen minorGridPen(gridColor, 0.35, Qt::DotLine);
    QPen majorGridPen(gridColor.darker(115), 0.7, Qt::DashLine);
    for (int x = 0; x <= viewWidth; x += minorTickInterval) {
        m_graphScene->addLine(x, 0, x, viewHeight, ((x % majorTickInterval) == 0) ? majorGridPen : minorGridPen);
    }
    for (int y = 0; y <= viewHeight; y += minorTickInterval) {
        m_graphScene->addLine(0, y, viewWidth, y, ((y % majorTickInterval) == 0) ? majorGridPen : minorGridPen);
    }
    
    // Set font for text items
    QFont textFont("MesloLGS NF", 12);
    textFont.setWeight(QFont::DemiBold);
    
    // Node dimensions - reduce margins to use more space
    int nodeWidth = 160;
    int nodeHeight = 60;
    int topMargin = 40;
    int leftMargin = 40;
    
    // Calculate positions - start from top-left with minimal margins
    int parentY = topMargin;
    int childY = topMargin + 150;
    
    // Place content near current visible region center while keeping large canvas
    QRectF visibleSceneRect = m_graphView->mapToScene(m_graphView->viewport()->rect()).boundingRect();
    int parentX = static_cast<int>(visibleSceneRect.center().x());
    if (parentX < viewWidth / 4 || parentX > (viewWidth * 3) / 4) {
        parentX = viewWidth / 2;
    }
    
    // Draw parent node
    QGraphicsRectItem* parentNode = m_graphScene->addRect(
        parentX - nodeWidth/2, parentY, nodeWidth, nodeHeight,
        QPen(borderColor, 2), QBrush(parentBgColor));
    
    QGraphicsTextItem* parentText = m_graphScene->addText(parent->name, textFont);
    parentText->setDefaultTextColor(textColor);
    qreal textHeight = parentText->boundingRect().height();
    qreal textWidth = parentText->boundingRect().width();
    parentText->setPos(parentX - textWidth/2, parentY + (nodeHeight - textHeight)/2);
    
    // Draw children nodes and arrows
    int availableWidth = viewWidth - 2 * leftMargin;
    int childSpacing = qMax(180, availableWidth / (children.size() + 1));
    int startX = parentX - (children.size() - 1) * childSpacing / 2;
    
    for (int i = 0; i < children.size(); ++i) {
        const R2moSubProject* child = children[i];
        int childX = startX + i * childSpacing;
        
        // Draw arrow from parent to child
        QGraphicsLineItem* line = m_graphScene->addLine(
            parentX, parentY + nodeHeight,
            childX, childY,
            QPen(arrowColor, 2));
        
        // Draw arrowhead
        QPolygonF arrowHead;
        arrowHead << QPointF(childX, childY)
                  << QPointF(childX - 10, childY - 14)
                  << QPointF(childX + 10, childY - 14);
        QGraphicsPolygonItem* arrow = m_graphScene->addPolygon(arrowHead, 
            QPen(arrowColor), QBrush(arrowColor));
        
        // Draw child node
        QGraphicsRectItem* childNode = m_graphScene->addRect(
            childX - nodeWidth/2, childY, nodeWidth, nodeHeight,
            QPen(QColor("#34c759"), 2), QBrush(childBgColor));
        
        QGraphicsTextItem* childText = m_graphScene->addText(child->name, textFont);
        childText->setDefaultTextColor(textColor);
        qreal childTextHeight = childText->boundingRect().height();
        qreal childTextWidth = childText->boundingRect().width();
        childText->setPos(childX - childTextWidth/2, childY + (nodeHeight - childTextHeight)/2);
    }
    
    // Add legend with proper text color
    QFont legendFont("MesloLGS NF", 10);
    QGraphicsTextItem* legend = m_graphScene->addText(
        tr("Ctrl+Scroll: Zoom | Drag: Pan | Double-click: Reset"), legendFont);
    legend->setDefaultTextColor(rulerTextColor);
    legend->setPos(12, 8);
    
    // Large scene rect for infinite-style expansion
    m_graphScene->setSceneRect(0, 0, viewWidth, viewHeight);
    
    // Center the view on the parent node
    QPointF centerPoint(parentX, parentY + nodeHeight / 2);
    m_graphView->centerOn(centerPoint);
}

void MainWindow::buildTaskTree(const QList<R2moSubProject>& projects)
{
    m_taskTree->clear();
    m_taskTree->setHeaderLabel(tr("Tasks by Project"));
    
    R2moScanner scanner;
    
    // Find parent project first
    const R2moSubProject* parent = nullptr;
    QList<const R2moSubProject*> children;
    
    for (const R2moSubProject& proj : projects) {
        if (proj.isParent) {
            parent = &proj;
        } else {
            children.append(&proj);
        }
    }
    
    if (!parent) return;
    
    // Create parent project item as top-level - use folder icon only, no triangle
    QTreeWidgetItem* parentItem = new QTreeWidgetItem(m_taskTree);
    parentItem->setText(0, QString("📁 %1").arg(parent->name));
    parentItem->setData(0, Qt::UserRole, parent->path);
    QFont parentFont = parentItem->font(0);
    parentFont.setBold(true);
    parentItem->setFont(0, parentFont);
    
    QString parentR2moPath = parent->path + "/.r2mo";
    
    // Add sub-projects under parent - use folder icon only, no triangle
    for (const R2moSubProject* child : children) {
        QTreeWidgetItem* childItem = new QTreeWidgetItem(parentItem);
        childItem->setText(0, QString("📂 %1").arg(child->name));
        childItem->setData(0, Qt::UserRole, child->path);
        
        QString childR2moPath = child->path + "/.r2mo";
        
        // Task Queue for sub-project
        QList<TaskInfo> childQueueTasks = scanner.getTaskQueueFiles(childR2moPath);
        QTreeWidgetItem* childQueueItem = new QTreeWidgetItem(childItem);
        childQueueItem->setText(0, tr("📋 Task Queue: %1 pending").arg(childQueueTasks.size()));
        childQueueItem->setForeground(0, QColor("#ff9500"));
        
        if (!childQueueTasks.isEmpty()) {
            for (const TaskInfo& task : childQueueTasks) {
                QTreeWidgetItem* taskItem = new QTreeWidgetItem(childQueueItem);
                QString displayText = task.fileName;
                if (!task.title.isEmpty() && task.title != task.fileName) {
                    displayText = QString("%1 - %2").arg(task.fileName, task.title);
                }
                taskItem->setText(0, displayText);
                taskItem->setData(0, Qt::UserRole, task.filePath);
                taskItem->setToolTip(0, task.modifiedTime.toString("yyyy-MM-dd hh:mm"));
            }
        }
        
        // Historical Tasks for sub-project
        QList<TaskInfo> childHistoricalTasks = scanner.getHistoricalTasks(childR2moPath);
        QTreeWidgetItem* childHistoryItem = new QTreeWidgetItem(childItem);
        childHistoryItem->setText(0, tr("📜 Historical Tasks: %1 completed").arg(childHistoricalTasks.size()));
        childHistoryItem->setForeground(0, QColor("#34c759"));
        
        if (!childHistoricalTasks.isEmpty()) {
            QMap<QString, QList<TaskInfo>> tasksByDate;
            for (const TaskInfo& task : childHistoricalTasks) {
                QString dateKey = task.runAtTime.isValid() 
                    ? task.runAtTime.toString("yyyy-MM-dd") 
                    : task.modifiedTime.toString("yyyy-MM-dd");
                tasksByDate[dateKey].append(task);
            }
            
            QList<QString> sortedDates = tasksByDate.keys();
            std::sort(sortedDates.begin(), sortedDates.end(), [](const QString& a, const QString& b) {
                return a > b;
            });
            
            for (const QString& date : sortedDates) {
                QTreeWidgetItem* dateItem = new QTreeWidgetItem(childHistoryItem);
                dateItem->setText(0, QString("📅 %1 (%2)").arg(date).arg(tasksByDate[date].size()));
                dateItem->setForeground(0, QColor("#86868b"));
                
                for (const TaskInfo& task : tasksByDate[date]) {
                    QTreeWidgetItem* taskItem = new QTreeWidgetItem(dateItem);
                    taskItem->setText(0, task.title);
                    taskItem->setData(0, Qt::UserRole, task.filePath);
                    QString timeStr = task.runAtTime.isValid() 
                        ? task.runAtTime.toString("yyyy-MM-dd hh:mm:ss") 
                        : task.modifiedTime.toString("yyyy-MM-dd hh:mm");
                    taskItem->setToolTip(0, QString("%1\n%2").arg(task.fileName, timeStr));
                }
            }
        }
        
        // Open button for sub-project
        QTreeWidgetItem* childOpenItem = new QTreeWidgetItem(childItem);
        childOpenItem->setText(0, tr("🔗 Open in Obsidian"));
        childOpenItem->setForeground(0, QColor("#007aff"));
        childOpenItem->setData(0, Qt::UserRole, child->path);
        
        childItem->setExpanded(true);
    }
    
    // Task Queue for parent project
    QList<TaskInfo> queueTasks = scanner.getTaskQueueFiles(parentR2moPath);
    QTreeWidgetItem* queueItem = new QTreeWidgetItem(parentItem);
    queueItem->setText(0, tr("📋 Task Queue: %1 pending").arg(queueTasks.size()));
    queueItem->setForeground(0, QColor("#ff9500"));
    
    if (!queueTasks.isEmpty()) {
        for (const TaskInfo& task : queueTasks) {
            QTreeWidgetItem* taskItem = new QTreeWidgetItem(queueItem);
            QString displayText = task.fileName;
            if (!task.title.isEmpty() && task.title != task.fileName) {
                displayText = QString("%1 - %2").arg(task.fileName, task.title);
            }
            taskItem->setText(0, displayText);
            taskItem->setData(0, Qt::UserRole, task.filePath);
            taskItem->setToolTip(0, task.modifiedTime.toString("yyyy-MM-dd hh:mm"));
        }
    }
    
    // Historical Tasks for parent project
    QList<TaskInfo> historicalTasks = scanner.getHistoricalTasks(parentR2moPath);
    QTreeWidgetItem* historyItem = new QTreeWidgetItem(parentItem);
    historyItem->setText(0, tr("📜 Historical Tasks: %1 completed").arg(historicalTasks.size()));
    historyItem->setForeground(0, QColor("#34c759"));
    
    if (!historicalTasks.isEmpty()) {
        QMap<QString, QList<TaskInfo>> tasksByDate;
        for (const TaskInfo& task : historicalTasks) {
            QString dateKey = task.runAtTime.isValid() 
                ? task.runAtTime.toString("yyyy-MM-dd") 
                : task.modifiedTime.toString("yyyy-MM-dd");
            tasksByDate[dateKey].append(task);
        }
        
        QList<QString> sortedDates = tasksByDate.keys();
        std::sort(sortedDates.begin(), sortedDates.end(), [](const QString& a, const QString& b) {
            return a > b;
        });
        
        for (const QString& date : sortedDates) {
            QTreeWidgetItem* dateItem = new QTreeWidgetItem(historyItem);
            dateItem->setText(0, QString("📅 %1 (%2)").arg(date).arg(tasksByDate[date].size()));
            dateItem->setForeground(0, QColor("#86868b"));
            
            for (const TaskInfo& task : tasksByDate[date]) {
                QTreeWidgetItem* taskItem = new QTreeWidgetItem(dateItem);
                taskItem->setText(0, task.title);
                taskItem->setData(0, Qt::UserRole, task.filePath);
                QString timeStr = task.runAtTime.isValid() 
                    ? task.runAtTime.toString("yyyy-MM-dd hh:mm:ss") 
                    : task.modifiedTime.toString("yyyy-MM-dd hh:mm");
                taskItem->setToolTip(0, QString("%1\n%2").arg(task.fileName, timeStr));
            }
        }
    }
    
    // Open button for parent project
    QTreeWidgetItem* openItem = new QTreeWidgetItem(parentItem);
    openItem->setText(0, tr("🔗 Open in Obsidian"));
    openItem->setForeground(0, QColor("#007aff"));
    openItem->setData(0, Qt::UserRole, parent->path);
    
    // Only expand the parent project node, not all nodes
    parentItem->setExpanded(true);
}

void MainWindow::onTaskItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    QString path = item->data(0, Qt::UserRole).toString();
    QString text = item->text(0);
    
    // Check if it's an "Open in Obsidian" item
    if (text.contains(tr("Open in Obsidian")) && !path.isEmpty()) {
        openVaultInObsidian(path);
        return;
    }
    
    // Check if it's a task file
    if (path.endsWith(".md") && QFile::exists(path)) {
        // Open the task file in default editor
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::onAIToolItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    const QString path = item->data(0, Qt::UserRole).toString();

    // AI tools tree supports opening files directly when double-clicked
    if (!path.isEmpty() && QFileInfo::exists(path) && QFileInfo(path).isFile()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::onPreviewEditClicked()
{
    if (m_currentPreviewPath.isEmpty()) return;
    
    // Switch to edit mode
    m_previewTitleEdit->setText(m_previewTitle->text());
    m_previewTitle->hide();
    m_previewEditBtn->hide();
    m_previewOpenBtn->hide();
    m_previewTitleEdit->show();
    m_previewTitleEdit->setFocus();
    m_previewTitleEdit->selectAll();
}

void MainWindow::onPreviewOpenClicked()
{
    if (m_currentPreviewPath.isEmpty()) return;
    openVaultInObsidian(m_currentPreviewPath);
}

void MainWindow::onPreviewTitleEditFinished()
{
    QString newName = m_previewTitleEdit->text().trimmed();
    
    // Switch back to display mode
    m_previewTitle->show();
    m_previewEditBtn->show();
    m_previewOpenBtn->show();
    m_previewTitleEdit->hide();
    
    if (newName.isEmpty() || m_currentPreviewPath.isEmpty()) {
        return;
    }
    
    QString oldName = m_previewTitle->text();
    if (newName == oldName) return;
    
    // Update the vault model
    int idx = m_vaultModel->indexOf(m_currentPreviewPath);
    if (idx >= 0) {
        Vault vault = m_vaultModel->vaultAt(idx);
        vault.name = newName;
        m_vaultModel->updateVault(m_currentPreviewPath, vault);
        m_vaultModel->save(m_configPath);
        
        // Update title label
        m_previewTitle->setText(newName);
    }
}

void MainWindow::onPreviewTitleReturnPressed()
{
    // Just finish editing - the editingFinished signal will handle the rest
    m_previewTitleEdit->clearFocus();
}

void MainWindow::onVaultSelected(QListWidgetItem* item)
{
    QString path = item->data(Qt::UserRole).toString();
    QString name = item->text();
    
    updatePreviewPane(name, path);
    
    // Save as last vault
    m_settingsManager->setLastVaultPath(path);
}

void MainWindow::onLanguageChanged(const QString& languageCode)
{
    Q_UNUSED(languageCode);
    updateLanguageButtons();
    retranslateUi();
}

void MainWindow::retranslateUi()
{
    // Disable updates to prevent flickering
    setUpdatesEnabled(false);
    
    // Retranslate window title
    setWindowTitle(tr("Obsidian Manager - Vault Center"));
    
    // Retranslate toolbar
    m_toolBar->setWindowTitle(tr("Toolbar"));
    
    // Retranslate theme toggle tooltip
    updateThemeToggleIcon();
    
    // Retranslate labels
    m_vaultLabel->setText(tr("Vaults"));
    m_previewLabel->setText(tr("Preview"));
    
    // Retranslate buttons
    m_addBtn->setText(tr("Add"));
    m_removeBtn->setText(tr("Remove"));
    m_previewOpenBtn->setText(tr("Open"));
    
    // Retranslate placeholder
    m_previewTitle->setText(tr("Select a vault..."));
    
    // Retranslate menu bar - just update action texts, don't rebuild
    QList<QAction*> actions = menuBar()->actions();
    for (QAction* action : actions) {
        if (action->menu()) {
            QMenu* menu = action->menu();
            QString menuTitle = menu->title();
            // Update menu title based on current text
            if (menuTitle.contains("File") || menuTitle.contains("文件")) {
                menu->setTitle(tr("File"));
            } else if (menuTitle.contains("Edit") || menuTitle.contains("编辑")) {
                menu->setTitle(tr("Edit"));
            } else if (menuTitle.contains("Help") || menuTitle.contains("帮助")) {
                menu->setTitle(tr("Help"));
            }
            
            // Update menu item texts
            for (QAction* menuAction : menu->actions()) {
                if (menuAction->isSeparator()) continue;
                QString actionText = menuAction->text();
                if (actionText.contains("Add") || actionText.contains("添加")) {
                    menuAction->setText(tr("Add Vault..."));
                } else if (actionText.contains("Remove") || actionText.contains("删除")) {
                    menuAction->setText(tr("Remove Vault"));
                } else if (actionText.contains("Open") || actionText.contains("打开")) {
                    menuAction->setText(tr("Open in Obsidian"));
                } else if (actionText.contains("Exit") || actionText.contains("退出")) {
                    menuAction->setText(tr("Exit"));
                } else if (actionText.contains("Settings") || actionText.contains("设置")) {
                    menuAction->setText(tr("Settings..."));
                } else if (actionText.contains("About") || actionText.contains("关于")) {
                    menuAction->setText(tr("About"));
                }
            }
        }
    }
    
    // Re-enable updates
    setUpdatesEnabled(true);
}

void MainWindow::onThemeChanged(ThemeManager::Theme theme)
{
    // Apply new theme style
    setStyleSheet(ThemeManager::instance()->currentStyle());
    updateThemeToggleIcon();
    updateToolbarIcons();
}

void MainWindow::onThemeToggle()
{
    ThemeManager::Theme current = ThemeManager::instance()->currentTheme();
    ThemeManager::Theme newTheme = (current == ThemeManager::Light) ? ThemeManager::Dark : ThemeManager::Light;
    ThemeManager::instance()->setTheme(newTheme);
    m_settingsManager->setTheme(static_cast<int>(newTheme));
}

void MainWindow::addSwimlaneCloseButton(int tabIndex)
{
    QToolButton *closeBtn = new QToolButton();
    closeBtn->setText("×");
    closeBtn->setFixedSize(QSize(14, 14));
    closeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QToolButton { background: transparent; color: #86868b; font-size: 12px; font-weight: bold; border: none; padding: 0; margin: 0; margin-right: 4px; } QToolButton:hover { background: #ff3b30; color: white; border-radius: 2px; }");
    connect(closeBtn, &QToolButton::clicked, this, [this]() {
        int idx = m_mainTabWidget->indexOf(m_swimlaneTabContent);
        if (idx > 0) {
            m_mainTabWidget->removeTab(idx);
            m_cachedSwimlaneWidget = m_swimlaneTabContent;
            m_swimlaneTabContent = nullptr;
        }
    });
    m_mainTabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, closeBtn);
}

void MainWindow::onSwimlane()
{
    if (m_cachedSwimlaneWidget) {
        m_swimlaneTabContent = m_cachedSwimlaneWidget;
        int idx = m_mainTabWidget->addTab(m_swimlaneTabContent, tr("Swimlane"));
        addSwimlaneCloseButton(idx);
        m_mainTabWidget->setCurrentIndex(idx);
        return;
    }

    if (m_swimlaneRefreshing) return;
    m_swimlaneRefreshing = true;

    // Create loading widget with progress animation
    QWidget *loadingWidget = new QWidget();
    loadingWidget->setStyleSheet("background: white;");
    QVBoxLayout *loadingLayout = new QVBoxLayout(loadingWidget);
    loadingLayout->setAlignment(Qt::AlignCenter);
    loadingLayout->setSpacing(16);
    
    // Animated loading icon
    QLabel *iconLabel = new QLabel("⏳");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("QLabel { font-size: 48px; background: transparent; }");
    loadingLayout->addWidget(iconLabel);
    
    // Main loading text
    QLabel *titleLabel = new QLabel(tr("Loading Swimlane View"));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #333; font-size: 18px; font-weight: 600; background: transparent; }");
    loadingLayout->addWidget(titleLabel);
    
    // Progress text (will be updated by timer)
    m_loadingProgressLabel = new QLabel(tr("Initializing..."));
    m_loadingProgressLabel->setAlignment(Qt::AlignCenter);
    m_loadingProgressLabel->setStyleSheet("QLabel { color: #86868b; font-size: 14px; background: transparent; }");
    loadingLayout->addWidget(m_loadingProgressLabel);
    
    // Progress bar (indeterminate)
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setRange(0, 0); // Indeterminate mode
    progressBar->setFixedWidth(300);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet("QProgressBar { border: 1px solid #e0e0e0; border-radius: 4px; background: #f5f5f7; height: 6px; } QProgressBar::chunk { background: #007aff; border-radius: 3px; }");
    loadingLayout->addWidget(progressBar);
    
    // Hint text
    QLabel *hintLabel = new QLabel(tr("Scanning Git repositories, AI tools, and task queues..."));
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet("QLabel { color: #86868b; font-size: 12px; background: transparent; }");
    loadingLayout->addWidget(hintLabel);
    
    m_swimlaneTabContent = loadingWidget;
    int newIdx = m_mainTabWidget->addTab(m_swimlaneTabContent, tr("Swimlane"));
    addSwimlaneCloseButton(newIdx);
    m_mainTabWidget->setCurrentIndex(newIdx);
    
    // Start progress animation
    m_loadingProgressStep = 0;
    m_loadingProgressTimer->start();
    
    QFuture<SwimlaneScanData> future = QtConcurrent::run([this]() {
        return this->collectSwimlaneData();
    });
    m_swimlaneScanWatcher->setFuture(future);
    
    if (m_swimlaneRefreshTimer) {
        m_swimlaneRefreshTimer->start();
    }
}

QWidget* MainWindow::buildSwimlaneView()
{
    SwimlaneScanData data = collectSwimlaneData();
    return buildSwimlaneView(data);
}

SwimlaneScanData MainWindow::collectSwimlaneData()
{
    SwimlaneScanData result;
    result.globalMaxQueue = 0;
    
    QList<Vault> vaults = m_vaultModel->vaults();
    
    // Cache Git scan results by actual repo path to avoid duplicate scans
    QMap<QString, GitStatusInfo> gitCache;
    
    auto getCachedGitStatus = [&](const QString& path) -> GitStatusInfo {
        // Find actual git repo root
        QString actualRepoPath = path;
        QDir dir(path);
        if (!dir.exists()) {
            GitStatusInfo empty;
            empty.isGitRepo = false;
            return empty;
        }
        
        QString gitDir = path + "/.git";
        if (!QDir(gitDir).exists()) {
            QDir checkDir(path);
            while (checkDir.cdUp()) {
                if (QDir(checkDir.path() + "/.git").exists()) {
                    actualRepoPath = checkDir.path();
                    break;
                }
                if (checkDir.isRoot()) break;
            }
        }
        
        if (gitCache.contains(actualRepoPath)) {
            return gitCache[actualRepoPath];
        }
        
        GitScanner gitScanner;
        GitStatusInfo status = gitScanner.scanRepository(path);
        gitCache[actualRepoPath] = status;
        return status;
    };
    
    // Cache AI tool scan results by project path
    QMap<QString, QList<AIToolInfo>> aiCache;
    
    auto getCachedAITools = [&](const QString& path) -> QList<AIToolInfo> {
        if (aiCache.contains(path)) {
            return aiCache[path];
        }
        AIToolScanner aiScanner;
        QList<AIToolInfo> tools = aiScanner.scanProject(path);
        aiCache[path] = tools;
        return tools;
    };
    
    for (const Vault& vault : vaults) {
        QDir vaultDir(vault.path);
        if (!vaultDir.exists() || !m_vaultValidator->hasR2moConfig(vault.path)) {
            continue;
        }
        R2moScanner scanner;
        QList<R2moSubProject> projects = scanner.scanVault(vault.path);
        if (projects.isEmpty()) continue;

        const R2moSubProject* parent = nullptr;
        QList<const R2moSubProject*> children;
        for (const R2moSubProject& proj : projects) {
            if (proj.isParent) parent = &proj;
            else children.append(&proj);
        }
        if (!parent) continue;

        SwimlaneVaultData vd;
        vd.vaultName = vault.name;
        vd.vaultPath = vault.path;
        
        vd.gitStatus = getCachedGitStatus(vault.path);
        
        QSet<QString> scannedTools;
        QList<AIToolInfo> allAiTools;
        
        QList<AIToolInfo> rootTools = getCachedAITools(vault.path);
        for (const AIToolInfo& tool : rootTools) {
            if (!scannedTools.contains(tool.name)) {
                allAiTools.append(tool);
                scannedTools.insert(tool.name);
            }
        }
        
        QList<AIToolInfo> parentTools = getCachedAITools(parent->path);
        for (const AIToolInfo& tool : parentTools) {
            if (!scannedTools.contains(tool.name)) {
                allAiTools.append(tool);
                scannedTools.insert(tool.name);
            }
        }
        
        for (const R2moSubProject* child : children) {
            QList<AIToolInfo> childTools = getCachedAITools(child->path);
            for (const AIToolInfo& tool : childTools) {
                if (!scannedTools.contains(tool.name)) {
                    allAiTools.append(tool);
                    scannedTools.insert(tool.name);
                }
            }
        }
        
        vd.aiTools = allAiTools;

        SwimlaneVaultData::LaneRow parentRow;
        parentRow.name = parent->name;
        parentRow.projectPath = parent->path;
        parentRow.isParent = true;
        parentRow.r2moPath = parent->path + "/.r2mo";
        parentRow.historicalCount = scanner.getHistoricalTasks(parent->path + "/.r2mo").size();
        parentRow.queueTasks = scanner.getTaskQueueFiles(parent->path + "/.r2mo");
        parentRow.gitStatus = getCachedGitStatus(parent->path);
        if (parentRow.queueTasks.size() > result.globalMaxQueue)
            result.globalMaxQueue = parentRow.queueTasks.size();

        for (const R2moSubProject* child : children) {
            SwimlaneVaultData::LaneRow childRow;
            childRow.name = child->name;
            childRow.projectPath = child->path;
            childRow.isParent = false;
            childRow.r2moPath = child->path + "/.r2mo";
            childRow.historicalCount = scanner.getHistoricalTasks(child->path + "/.r2mo").size();
            childRow.queueTasks = scanner.getTaskQueueFiles(child->path + "/.r2mo");
            childRow.gitStatus = getCachedGitStatus(child->path);
            if (childRow.queueTasks.size() > result.globalMaxQueue)
                result.globalMaxQueue = childRow.queueTasks.size();
            parentRow.children.append(childRow);
        }
        vd.rows.append(parentRow);
        result.vaults.append(vd);
    }
    
    return result;
}

QWidget* MainWindow::buildSwimlaneView(const SwimlaneScanData& scanData)
{
    QWidget *container = new QWidget();
    container->setStyleSheet("background: white;");
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(8);

    QList<Vault> vaults = m_vaultModel->vaults();
    if (vaults.isEmpty()) {
        QLabel *emptyLabel = new QLabel(tr("No vaults added. Add a vault to see swimlane view."));
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("QLabel { color: #86868b; font-size: 15px; padding: 40px; background: white; }");
        mainLayout->addWidget(emptyLabel);
        return container;
    }

    // Legend + Refresh button
    QHBoxLayout *legendLayout = new QHBoxLayout();
    QLabel *legendLabel = new QLabel(tr("Running Tasks"));
    legendLabel->setStyleSheet("QLabel { color: #86868b; font-size: 11px; background: white; }");
    legendLayout->addWidget(legendLabel);
    
    QPushButton *refreshBtn = new QPushButton(tr("Refresh"));
    refreshBtn->setFixedSize(QSize(80, 24));
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet("QPushButton { background: #f5f5f7; color: #86868b; font-size: 11px; border: 1px solid #e0e0e0; border-radius: 4px; padding: 2px 8px; } QPushButton:hover { background: #e8e8ed; color: #333; } QPushButton:disabled { background: #f5f5f7; color: #c0c0c0; }");
    refreshBtn->setProperty("isRefreshBtn", true);
    legendLayout->addWidget(refreshBtn);
    legendLayout->addStretch();
    mainLayout->addLayout(legendLayout);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #e0e0e0; background: white; border-radius: 4px; }");

    QWidget *swimlaneContent = new QWidget();
    swimlaneContent->setStyleSheet("background: white;");
    QVBoxLayout *contentLayout = new QVBoxLayout(swimlaneContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    QColor borderColor("#e0e0e0");
    QColor textColor("#333333");
    QColor headerBg("#f5f5f7");
    QColor laneBg("#ffffff");

    const QList<SwimlaneVaultData>& allVaultData = scanData.vaults;
    int globalMaxQueue = scanData.globalMaxQueue;

    if (allVaultData.isEmpty()) {
        QLabel *noDataLabel = new QLabel(tr("No R2MO projects found in any vault."));
        noDataLabel->setAlignment(Qt::AlignCenter);
        noDataLabel->setStyleSheet("QLabel { color: #86868b; font-size: 15px; padding: 40px; background: white; }");
        contentLayout->addWidget(noDataLabel);
        scrollArea->setWidget(swimlaneContent);
        mainLayout->addWidget(scrollArea, 1);
        
        QPushButton *btn = container->findChild<QPushButton*>("", Qt::FindDirectChildrenOnly);
        if (btn && btn->property("isRefreshBtn").toBool()) {
            connect(btn, &QPushButton::clicked, this, &MainWindow::refreshSwimlaneAsync);
        }
        return container;
    }

    int nameColWidth = 260;
    int histColWidth = 40;
    int cellWidth = 36;
    int cellHeight = 24;
    int rowHeight = 28;
    int gridColCount = qMax(1, globalMaxQueue);

    QColor runningColor("#007aff");
    QColor emptyBorder("#d0d0d0");

    auto addLaneRow = [&](QGridLayout *grid, int row, const QString &name,
                          int histCount, const QList<TaskInfo> &queueTasks,
                          const QString &r2moPath, bool isChild,
                          const GitStatusInfo &gitStatus) {
        QString nameText = isChild ? QString("  └ %1").arg(name) : name;
        QLabel *nameLabel = new QLabel(nameText);
        nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameLabel->setFixedSize(nameColWidth, rowHeight);
        bool hasChanges = (gitStatus.isGitRepo && 
                          (gitStatus.behindCount > 0 ||
                           gitStatus.stagedCount > 0 || gitStatus.modifiedCount > 0 || 
                           gitStatus.untrackedCount > 0));
        QString nameColor = hasChanges ? "#8b2500" : "#2e7d32";
        nameLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; padding: 2px 4px; background: %2; border-bottom: 1px solid %3; }")
            .arg(nameColor).arg(laneBg.name()).arg(borderColor.name()));
        if (!isChild) {
            QFont f = nameLabel->font();
            f.setBold(true);
            nameLabel->setFont(f);
        }
        grid->addWidget(nameLabel, row, 0);

        if (histCount > 0) {
            QLabel *histLabel = new QLabel(QString::number(histCount));
            histLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            histLabel->setFixedSize(histColWidth, rowHeight);
            histLabel->setCursor(Qt::PointingHandCursor);
            histLabel->setStyleSheet(QString("QLabel { color: #007aff; font-size: 14px; padding: 2px 4px; background: %1; border-bottom: 1px solid %2; text-decoration: underline; }").arg(laneBg.name()).arg(borderColor.name()));
            histLabel->setProperty("r2moPath", r2moPath);
            histLabel->setProperty("projectName", name);
            histLabel->installEventFilter(this);
            grid->addWidget(histLabel, row, 1);
        } else {
            QLabel *emptyHist = new QLabel("");
            emptyHist->setFixedSize(histColWidth, rowHeight);
            emptyHist->setStyleSheet(QString("QLabel { background: %1; border-bottom: 1px solid %2; }").arg(laneBg.name()).arg(borderColor.name()));
            grid->addWidget(emptyHist, row, 1);
        }

        for (int col = 0; col < gridColCount; ++col) {
            QFrame *cell = new QFrame();
            cell->setFixedSize(cellWidth, cellHeight);
            if (col < queueTasks.size()) {
                cell->setStyleSheet(QString("QFrame { background: %1; border: 1px solid %2; border-radius: 3px; margin: 1px; }").arg(runningColor.name()).arg(runningColor.darker(120).name()));
                cell->setToolTip(queueTasks[col].title.isEmpty() ? queueTasks[col].fileName : queueTasks[col].title);
            } else {
                cell->setStyleSheet(QString("QFrame { background: transparent; border: 1px solid %1; border-radius: 3px; margin: 1px; }").arg(emptyBorder.name()));
            }
            grid->addWidget(cell, row, col + 2);
        }
    };

    for (const SwimlaneVaultData &vd : allVaultData) {
        QWidget *vaultHeaderWidget = new QWidget();
        QHBoxLayout *vaultHeaderLayout = new QHBoxLayout(vaultHeaderWidget);
        vaultHeaderLayout->setContentsMargins(8, 4, 8, 4);
        vaultHeaderLayout->setSpacing(8);
        vaultHeaderWidget->setStyleSheet(QString("QWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #e8f4ff, stop:0.5 #f0e8ff, stop:1 #fff8f0); border: none; border-radius: 6px; }"));
        
        QLabel *vaultNameLabel = new QLabel(QString("📁 %1").arg(vd.vaultName));
        vaultNameLabel->setStyleSheet("QLabel { color: #af52de; font-size: 14px; font-weight: 600; background: transparent; border: none; }");
        vaultHeaderLayout->addWidget(vaultNameLabel);
        
        for (const AIToolInfo& tool : vd.aiTools) {
            QWidget *toolWidget = new QWidget();
            QHBoxLayout *toolLayout = new QHBoxLayout(toolWidget);
            toolLayout->setContentsMargins(0, 0, 0, 0);
            toolLayout->setSpacing(4);
            
            QLabel *iconLabel = new QLabel();
            if (!tool.iconPath.isEmpty() && QFile::exists(tool.iconPath)) {
                QPixmap pixmap(tool.iconPath);
                if (!pixmap.isNull()) {
                    iconLabel->setPixmap(pixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
            iconLabel->setStyleSheet("QLabel { background: transparent; border: none; }");
            toolLayout->addWidget(iconLabel);
            
            QLabel *countLabel = new QLabel(QString::number(tool.sessions.count));
            countLabel->setStyleSheet("QLabel { color: #86868b; font-size: 12px; background: transparent; border: none; }");
            toolLayout->addWidget(countLabel);
            
            toolWidget->setToolTip(QString("%1: %2 sessions").arg(tool.name).arg(tool.sessions.count));
            
            vaultHeaderLayout->addWidget(toolWidget);
        }
        
        vaultHeaderLayout->addStretch();
        
        if (vd.gitStatus.isGitRepo) {
            QWidget *gitWidget = new QWidget();
            QHBoxLayout *gitLayout = new QHBoxLayout(gitWidget);
            gitLayout->setContentsMargins(0, 0, 0, 0);
            gitLayout->setSpacing(6);
            
            bool hasChanges = (vd.gitStatus.behindCount > 0 ||
                              vd.gitStatus.stagedCount > 0 || vd.gitStatus.modifiedCount > 0 || 
                              vd.gitStatus.untrackedCount > 0);
            
            QString branchText = hasChanges ? QString("★ %1").arg(vd.gitStatus.branch) : 
                                  (vd.gitStatus.aheadCount > 0 ? QString("↑ %1").arg(vd.gitStatus.branch) : vd.gitStatus.branch);
            QString branchColor = hasChanges ? "#b8860b" : "#34c759";
            QLabel *branchLabel = new QLabel(branchText);
            branchLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; background: transparent; border: none; font-weight: 600; }").arg(branchColor));
            gitLayout->addWidget(branchLabel);
            
            QString markerBase = "QLabel { font-size: 14px; background: transparent; border: none; font-weight: 600; ";
            
            QLabel *aheadLabel = new QLabel(QString("↑%1").arg(vd.gitStatus.aheadCount));
            aheadLabel->setStyleSheet(markerBase + "color: #007aff; min-width: 28px; }");
            gitLayout->addWidget(aheadLabel);
            
            QLabel *behindLabel = new QLabel(QString("↓%1").arg(vd.gitStatus.behindCount));
            behindLabel->setStyleSheet(markerBase + "color: #ff9500; min-width: 28px; }");
            gitLayout->addWidget(behindLabel);
            
            QLabel *stagedLabel = new QLabel(QString("!%1").arg(vd.gitStatus.stagedCount));
            stagedLabel->setStyleSheet(markerBase + "color: #34c759; min-width: 24px; }");
            gitLayout->addWidget(stagedLabel);
            
            QLabel *modifiedLabel = new QLabel(QString("✗%1").arg(vd.gitStatus.modifiedCount));
            modifiedLabel->setStyleSheet(markerBase + "color: #ff3b30; min-width: 24px; }");
            gitLayout->addWidget(modifiedLabel);
            
            QLabel *untrackedLabel = new QLabel(QString("?%1").arg(vd.gitStatus.untrackedCount));
            untrackedLabel->setStyleSheet(markerBase + "color: #86868b; min-width: 24px; }");
            gitLayout->addWidget(untrackedLabel);
            
            vaultHeaderLayout->addWidget(gitWidget);
        }
        
        contentLayout->addWidget(vaultHeaderWidget);

        QWidget *gridWidget = new QWidget();
        gridWidget->setStyleSheet("background: white;");
        QGridLayout *grid = new QGridLayout(gridWidget);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(0);

        QLabel *nameHeader = new QLabel(tr("Project"));
        nameHeader->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameHeader->setFixedHeight(24);
        nameHeader->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; font-weight: 600; padding: 2px 4px; background: %2; border-bottom: 1px solid %3; }").arg(textColor.name()).arg(headerBg.name()).arg(borderColor.name()));
        grid->addWidget(nameHeader, 0, 0);

        QLabel *histHeader = new QLabel(tr("Hist."));
        histHeader->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        histHeader->setFixedSize(histColWidth, 24);
        histHeader->setToolTip(tr("Historical task count"));
        histHeader->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; font-weight: 600; padding: 2px 4px; background: %2; border-bottom: 1px solid %3; }").arg(textColor.name()).arg(headerBg.name()).arg(borderColor.name()));
        grid->addWidget(histHeader, 0, 1);

        for (int col = 0; col < gridColCount; ++col) {
            QLabel *colH = new QLabel(QString("#%1").arg(col + 1));
            colH->setAlignment(Qt::AlignCenter);
            colH->setFixedSize(cellWidth, 24);
            colH->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; font-weight: 600; background: %2; border-bottom: 1px solid %3; }").arg(textColor.name()).arg(headerBg.name()).arg(borderColor.name()));
            grid->addWidget(colH, 0, col + 2);
        }

        grid->setColumnStretch(gridColCount + 2, 1);

        int rowIdx = 1;
        for (const SwimlaneVaultData::LaneRow &parentRow : vd.rows) {
            addLaneRow(grid, rowIdx++, parentRow.name, parentRow.historicalCount, parentRow.queueTasks, parentRow.r2moPath, false, parentRow.gitStatus);
            for (const SwimlaneVaultData::LaneRow &childRow : parentRow.children) {
                addLaneRow(grid, rowIdx++, childRow.name, childRow.historicalCount, childRow.queueTasks, childRow.r2moPath, true, childRow.gitStatus);
            }
        }

        contentLayout->addWidget(gridWidget);
        contentLayout->addSpacing(12);
    }

    contentLayout->addStretch(1);
    scrollArea->setWidget(swimlaneContent);
    mainLayout->addWidget(scrollArea, 1);

    QPushButton *btn = container->findChild<QPushButton*>("", Qt::FindDirectChildrenOnly);
    if (btn && btn->property("isRefreshBtn").toBool()) {
        connect(btn, &QPushButton::clicked, this, &MainWindow::refreshSwimlaneAsync);
    }

    return container;
}

void MainWindow::refreshSwimlaneAsync()
{
    if (m_swimlaneRefreshing) return;
    if (!m_swimlaneTabContent) return;
    
    int idx = m_mainTabWidget->indexOf(m_swimlaneTabContent);
    if (idx < 0) return;
    
    m_swimlaneRefreshing = true;
    
    // Create loading widget with progress animation
    QWidget *loadingOverlay = new QWidget();
    loadingOverlay->setStyleSheet("background: rgba(255, 255, 255, 0.95);");
    QVBoxLayout *overlayLayout = new QVBoxLayout(loadingOverlay);
    overlayLayout->setAlignment(Qt::AlignCenter);
    overlayLayout->setSpacing(16);
    
    // Animated loading icon
    QLabel *iconLabel = new QLabel("🔄");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("QLabel { font-size: 48px; background: transparent; }");
    overlayLayout->addWidget(iconLabel);
    
    // Main loading text
    QLabel *titleLabel = new QLabel(tr("Refreshing Swimlane View"));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #333; font-size: 18px; font-weight: 600; background: transparent; }");
    overlayLayout->addWidget(titleLabel);
    
    // Progress text (will be updated by timer)
    m_loadingProgressLabel = new QLabel(tr("Initializing..."));
    m_loadingProgressLabel->setAlignment(Qt::AlignCenter);
    m_loadingProgressLabel->setStyleSheet("QLabel { color: #86868b; font-size: 14px; background: transparent; }");
    overlayLayout->addWidget(m_loadingProgressLabel);
    
    // Progress bar (indeterminate)
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setRange(0, 0);
    progressBar->setFixedWidth(300);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet("QProgressBar { border: 1px solid #e0e0e0; border-radius: 4px; background: #f5f5f7; height: 6px; } QProgressBar::chunk { background: #007aff; border-radius: 3px; }");
    overlayLayout->addWidget(progressBar);
    
    m_cachedSwimlaneWidget = m_swimlaneTabContent;
    m_mainTabWidget->removeTab(idx);
    m_swimlaneTabContent = loadingOverlay;
    int newIdx = m_mainTabWidget->addTab(m_swimlaneTabContent, tr("Swimlane"));
    addSwimlaneCloseButton(newIdx);
    m_mainTabWidget->setCurrentIndex(newIdx);
    
    // Start progress animation
    m_loadingProgressStep = 0;
    m_loadingProgressTimer->start();
    
    QFuture<SwimlaneScanData> future = QtConcurrent::run([this]() {
        return this->collectSwimlaneData();
    });
    m_swimlaneScanWatcher->setFuture(future);
}

void MainWindow::showHistoricalTasksDialog(const QString& r2moPath, const QString& projectName)
{
    R2moScanner scanner;
    QList<TaskInfo> tasks = scanner.getHistoricalTasks(r2moPath);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Historical Tasks - %1").arg(projectName));
    dialog.resize(1120, 620);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTreeWidget *tree = new QTreeWidget(&dialog);
    tree->setHeaderLabels(QStringList() << tr("Task") << tr("Time") << tr("File"));
    tree->setRootIsDecorated(true);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(false);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    if (tasks.isEmpty()) {
        QTreeWidgetItem *empty = new QTreeWidgetItem(tree);
        empty->setText(0, tr("No historical tasks found."));
        tree->addTopLevelItem(empty);
    } else {
        QMap<QString, QList<TaskInfo>> grouped;
        for (const TaskInfo &task : tasks) {
            QString dateKey;
            QString p = task.filePath;
            int idx = p.indexOf("/task/");
            if (idx >= 0) {
                QString rel = p.mid(idx + 6);
                int slash = rel.indexOf('/');
                if (slash > 0) {
                    QString d = rel.left(slash);
                    static const QRegularExpression dateRe("^\\d{4}-\\d{2}-\\d{2}$");
                    if (dateRe.match(d).hasMatch()) {
                        dateKey = d;
                    }
                }
            }
            if (dateKey.isEmpty()) {
                dateKey = task.modifiedTime.date().toString("yyyy-MM-dd");
            }
            grouped[dateKey].append(task);
        }

        QStringList dates = grouped.keys();
        std::sort(dates.begin(), dates.end(), std::greater<QString>());

        for (const QString &date : dates) {
            QTreeWidgetItem *dateItem = new QTreeWidgetItem(tree);
            dateItem->setText(0, QString("%1 (%2)").arg(date).arg(grouped[date].size()));
            QFont f = dateItem->font(0);
            f.setBold(true);
            dateItem->setFont(0, f);

            for (const TaskInfo &task : grouped[date]) {
                QTreeWidgetItem *item = new QTreeWidgetItem(dateItem);
                item->setText(0, task.title);
                QDateTime t = task.runAtTime.isValid() ? task.runAtTime : task.modifiedTime;
                item->setText(1, t.toString("yyyy-MM-dd HH:mm:ss"));
                item->setText(2, task.fileName);
                item->setToolTip(0, task.filePath);
            }
            dateItem->setExpanded(true);
            tree->addTopLevelItem(dateItem);
        }
    }

    layout->addWidget(tree, 1);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    QPushButton *closeBtn = new QPushButton(tr("Close"), &dialog);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    dialog.exec();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Obsidian Manager"),
                       tr("<h2>Obsidian Manager</h2>"
                          "<p style='color: #86868b;'>Version 1.0.0</p>"
                          "<p>A centralized vault management tool for Obsidian.</p>"
                          "<p style='margin-top: 16px;'><b>Features:</b></p>"
                          "<ul>"
                          "<li>Manage multiple vaults</li>"
                          "<li>Validate Obsidian vault structure</li>"
                          "<li>Show hidden directories (.obsidian, .r2mo, etc.)</li>"
                          "<li>Quick vault switching</li>"
                          "<li>Open vaults in Obsidian</li>"
                          "<li>Multi-language support (EN/ZH)</li>"
                          "</ul>"
                          "<p style='color: #86868b; margin-top: 16px;'>Built with Qt 6.10.0</p>"));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched->property("isProjectSelectionTreeViewport").toBool() &&
        (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (QTreeWidget *tree = qobject_cast<QTreeWidget*>(watched->parent())) {
                QTreeWidgetItem *item = tree->itemAt(mouseEvent->pos());
                if (item) {
                    const bool expandable = item->data(0, Qt::UserRole + 4).toBool();
                    if (expandable) {
                        const QRect itemRect = tree->visualRect(tree->indexFromItem(item, 0));
                        const int iconHotspotRight = itemRect.left() + tree->iconSize().width() + 10;
                        if (mouseEvent->pos().x() <= iconHotspotRight) {
                            item->setExpanded(!item->isExpanded());
                            tree->setCurrentItem(item);
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (QTreeWidget *tree = qobject_cast<QTreeWidget*>(watched)) {
        if (tree->property("isMonitorTree").toBool() &&
            (event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest)) {
            updateMonitorTableColumns(tree);
        }
    }

    if (m_specialMonitorTable && watched == m_specialMonitorTable->viewport() &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest)) {
        updateSpecialMonitorTableColumns();
    }

    // Handle wheel events for graph view zoom
    if (watched == m_graphView && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        
        // Zoom with Ctrl + Wheel
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            const double scaleFactor = 1.15;
            if (wheelEvent->angleDelta().y() > 0) {
                m_graphView->scale(scaleFactor, scaleFactor);
            } else {
                m_graphView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            }
            return true;
        }
    }
    
    // Handle double-click to reset zoom
    if (watched == m_graphView && event->type() == QEvent::MouseButtonDblClick) {
        m_graphView->resetTransform();
        m_graphView->scale(1.0, 1.0);
        return true;
    }

    // Handle click on historical count labels in swimlane
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label && label->property("r2moPath").isValid()) {
            const QString r2moPath = label->property("r2moPath").toString();
            const QString projectName = label->property("projectName").toString();
            if (!r2moPath.isEmpty()) {
                showHistoricalTasksDialog(r2moPath, projectName.isEmpty() ? tr("Project") : projectName);
                return true;
            }
        }
    }
    
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::buildAIToolsTab(const QString& vaultPath)
{
    m_aiToolsTree->clear();
    
    if (vaultPath.trimmed().isEmpty()) {
        m_aiToolsEmptyLabel->setVisible(true);
        m_aiToolsTree->setVisible(false);
        return;
    }
    
    // Use R2moScanner to get project structure (same as Task Tree)
    R2moScanner scanner;
    QList<R2moSubProject> projects = scanner.scanVault(vaultPath);
    
    if (projects.isEmpty()) {
        m_aiToolsEmptyLabel->setText(tr("No R2MO projects found."));
        m_aiToolsEmptyLabel->setVisible(true);
        m_aiToolsTree->setVisible(false);
        return;
    }
    
    m_aiToolsEmptyLabel->setVisible(false);
    m_aiToolsTree->setVisible(true);
    
    buildAIToolsTree(projects);
}

void MainWindow::buildAIToolsTree(const QList<R2moSubProject>& projects)
{
    m_aiToolsTree->clear();
    m_aiToolsTree->setHeaderLabel(tr("AI Tool Configurations"));
    
    // Find parent and children (same pattern as buildTaskTree)
    const R2moSubProject* parent = nullptr;
    QList<const R2moSubProject*> children;
    
    for (const R2moSubProject& proj : projects) {
        if (proj.isParent) {
            parent = &proj;
        } else {
            children.append(&proj);
        }
    }
    
    if (!parent) return;
    
    AIToolScanner aiScanner;
    
    // Build parent node off-tree first, only attach when it has real AI tools
    QTreeWidgetItem* parentItem = new QTreeWidgetItem();
    parentItem->setText(0, QString("📁 %1").arg(parent->name));
    QFont parentFont = parentItem->font(0);
    parentFont.setBold(true);
    parentItem->setFont(0, parentFont);
    parentItem->setData(0, Qt::UserRole, parent->path);

    bool parentHasAnyTools = false;
    
    // Add sub-projects under parent FIRST (like Task Tree)
    for (const R2moSubProject* child : children) {
        QTreeWidgetItem* childItem = new QTreeWidgetItem();
        childItem->setText(0, QString("📂 %1").arg(child->name));
        childItem->setData(0, Qt::UserRole, child->path);
        
        // Add AI tools for sub-project
        QList<AIToolInfo> childTools = aiScanner.scanProject(child->path);
        const bool childHasTools = addAIToolsToItem(childItem, childTools);
        if (childHasTools) {
            parentItem->addChild(childItem);
            parentHasAnyTools = true;
        } else {
            delete childItem;
        }
    }
    
    // Add AI tools for parent project AFTER sub-projects
    QList<AIToolInfo> parentTools = aiScanner.scanProject(parent->path);
    parentHasAnyTools = addAIToolsToItem(parentItem, parentTools) || parentHasAnyTools;

    if (parentHasAnyTools) {
        m_aiToolsTree->addTopLevelItem(parentItem);
        // Align with task tree: default expand root project node
        parentItem->setExpanded(true);
    } else {
        delete parentItem;
    }
}

bool MainWindow::addAIToolsToItem(QTreeWidgetItem* parentItem, const QList<AIToolInfo>& tools)
{
    if (tools.isEmpty()) {
        return false;
    }

    bool hasAnyTools = false;
    
    for (const AIToolInfo& tool : tools) {
        int totalCount = tool.sessions.count + tool.skills.count + tool.rules.count + tool.mcp.count;
        if (totalCount == 0) {
            continue;
        }

        QTreeWidgetItem* toolItem = new QTreeWidgetItem(parentItem);
        
        // Build display text with counts
        QStringList counts;
        if (tool.sessions.count > 0) counts << tr("%1 sessions").arg(tool.sessions.count);
        if (tool.skills.count > 0) counts << tr("%1 skills").arg(tool.skills.count);
        if (tool.rules.count > 0) counts << tr("%1 rules").arg(tool.rules.count);
        if (tool.mcp.count > 0) counts << tr("%1 mcp").arg(tool.mcp.count);
        
        QString displayText = tool.name;
        if (!counts.isEmpty()) {
            displayText += QString(" (%1)").arg(counts.join(", "));
        }
        
        toolItem->setText(0, displayText);
        toolItem->setData(0, Qt::UserRole, tool.configDir);
        
        // Set icon if available
        if (!tool.iconPath.isEmpty() && QFile::exists(tool.iconPath)) {
            QPixmap pixmap(tool.iconPath);
            if (!pixmap.isNull()) {
                toolItem->setIcon(0, pixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
        
        // Add categories
        auto addCategory = [&](const AIToolCategory& category, const QColor& color) {
            if (category.count == 0) return;
            
            QTreeWidgetItem* catItem = new QTreeWidgetItem(toolItem);
            catItem->setText(0, QString("%1 %2: %3").arg(category.icon, category.name).arg(category.count));
            catItem->setForeground(0, color);
            
            for (const AIToolItem& item : category.items) {
                QTreeWidgetItem* fileItem = new QTreeWidgetItem(catItem);
                fileItem->setText(0, item.displayText);
                fileItem->setData(0, Qt::UserRole, item.path);
                if (item.modifiedTime.isValid()) {
                    fileItem->setToolTip(0, QString("%1\n%2").arg(item.path, item.modifiedTime.toString("yyyy-MM-dd hh:mm")));
                } else {
                    fileItem->setToolTip(0, item.path);
                }
            }
        };
        
        addCategory(tool.sessions, QColor("#007aff"));
        addCategory(tool.skills, QColor("#ff9500"));
        addCategory(tool.rules, QColor("#34c759"));
        addCategory(tool.mcp, QColor("#af52de"));
        
        // Keep tool node collapsed by default
        toolItem->setExpanded(false);
        hasAnyTools = true;
    }

    return hasAnyTools;
}

void MainWindow::onSwimlaneRefresh()
{
    if (m_swimlaneRefreshing) return;
    
    m_swimlaneRefreshing = true;
    
    QFuture<SwimlaneScanData> future = QtConcurrent::run([this]() {
        return this->collectSwimlaneData();
    });
    m_swimlaneScanWatcher->setFuture(future);
}

void MainWindow::addMonitorCloseButton(int tabIndex)
{
    QToolButton *closeBtn = new QToolButton();
    closeBtn->setText("×");
    closeBtn->setFixedSize(QSize(14, 14));
    closeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QToolButton { background: transparent; color: #86868b; font-size: 12px; font-weight: bold; border: none; padding: 0; margin: 0; margin-right: 4px; } QToolButton:hover { background: #ff3b30; color: white; border-radius: 2px; }");
    connect(closeBtn, &QToolButton::clicked, this, [this]() {
        int idx = m_mainTabWidget->indexOf(m_monitorTabContent);
        if (idx > 0) {
            m_mainTabWidget->removeTab(idx);
            m_cachedMonitorWidget = m_monitorTabContent;
            m_monitorTabContent = nullptr;
        }
    });
    m_mainTabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, closeBtn);
}

QList<QPair<QString, QString>> MainWindow::collectAllProjectPaths()
{
    QList<QPair<QString, QString>> result;
    QSet<QString> seenPaths;
    QList<Vault> vaults = m_vaultModel->vaults();
    
    for (const Vault& vault : vaults) {
        QDir vaultDir(vault.path);
        if (!vaultDir.exists()) {
            continue;
        }

        const QString normalizedVaultPath = QDir::cleanPath(vault.path);
        if (!seenPaths.contains(normalizedVaultPath)) {
            result.append(qMakePair(vault.name, normalizedVaultPath));
            seenPaths.insert(normalizedVaultPath);
        }

        if (!m_vaultValidator->hasR2moConfig(vault.path)) {
            continue;
        }

        R2moScanner scanner;
        QList<R2moSubProject> projects = scanner.scanVault(vault.path);
        for (const R2moSubProject& proj : projects) {
            const QString normalizedProjectPath = QDir::cleanPath(proj.path);
            if (seenPaths.contains(normalizedProjectPath)) {
                continue;
            }
            result.append(qMakePair(proj.name, normalizedProjectPath));
            seenPaths.insert(normalizedProjectPath);
        }
    }
    
    return result;
}

void MainWindow::onMonitorBoard()
{
    if (m_cachedMonitorWidget) {
        m_monitorTabContent = m_cachedMonitorWidget;
        int idx = m_mainTabWidget->addTab(m_monitorTabContent, tr("Monitor Board"));
        addMonitorCloseButton(idx);
        m_mainTabWidget->setCurrentIndex(idx);
        return;
    }
    
    openMonitorTab();
}

void MainWindow::openMonitorTab()
{
    if (m_monitorRefreshing) return;
    m_monitorRefreshing = true;
    
    QWidget *loadingWidget = new QWidget();
    loadingWidget->setStyleSheet("background: white;");
    QVBoxLayout *loadingLayout = new QVBoxLayout(loadingWidget);
    loadingLayout->setAlignment(Qt::AlignCenter);
    loadingLayout->setSpacing(16);
    
    QLabel *iconLabel = new QLabel();
    iconLabel->setAlignment(Qt::AlignCenter);
    QPixmap monitorPix(":/icons/monitor.svg");
    if (!monitorPix.isNull()) {
        iconLabel->setPixmap(monitorPix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    loadingLayout->addWidget(iconLabel);
    
    QLabel *titleLabel = new QLabel(tr("Loading Monitor Board"));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #333; font-size: 18px; font-weight: 600; background: transparent; }");
    loadingLayout->addWidget(titleLabel);
    
    m_monitorProgressLabel = new QLabel(tr("Scanning AI sessions..."));
    m_monitorProgressLabel->setAlignment(Qt::AlignCenter);
    m_monitorProgressLabel->setStyleSheet("QLabel { color: #86868b; font-size: 14px; background: transparent; }");
    loadingLayout->addWidget(m_monitorProgressLabel);
    
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setRange(0, 0);
    progressBar->setFixedWidth(300);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet("QProgressBar { border: 1px solid #e0e0e0; border-radius: 4px; background: #f5f5f7; height: 6px; } QProgressBar::chunk { background: #34c759; border-radius: 3px; }");
    loadingLayout->addWidget(progressBar);
    
    m_monitorTabContent = loadingWidget;
    int newIdx = m_mainTabWidget->addTab(m_monitorTabContent, tr("Monitor Board"));
    addMonitorCloseButton(newIdx);
    m_mainTabWidget->setCurrentIndex(newIdx);
    
    QFuture<QList<ProjectMonitorData>> future = QtConcurrent::run([this]() {
        QList<QPair<QString, QString>> projects = collectAllProjectPaths();
        SessionScanner scanner;
        return scanner.scanLiveSessions(projects);
    });
    m_monitorScanWatcher->setFuture(future);
    
    if (m_monitorRefreshTimer) {
        m_monitorRefreshTimer->start();
    }
}

void MainWindow::onMonitorRefresh()
{
    if (m_monitorRefreshing) return;
    
    m_monitorRefreshing = true;
    
    QFuture<QList<ProjectMonitorData>> future = QtConcurrent::run([this]() {
        QList<QPair<QString, QString>> projects = collectAllProjectPaths();
        SessionScanner scanner;
        return scanner.scanLiveSessions(projects);
    });
    m_monitorScanWatcher->setFuture(future);
}

void MainWindow::openMonitorTarget(QTreeWidgetItem *row)
{
    if (!row) {
        return;
    }

    const qint64 terminalPid = row->data(0, Qt::UserRole + 2).toLongLong();
    if (terminalPid > 0 && activateTerminalWindow(terminalPid)) {
        return;
    }

    const QString projectPath = row->data(0, Qt::UserRole).toString();
    if (projectPath.isEmpty()) {
        return;
    }

    QString preferredOpenPath = projectPath;
    const QList<Vault> vaults = m_vaultModel->vaults();
    for (const Vault& vault : vaults) {
        const QString vaultPath = QDir::cleanPath(vault.path);
        const QString normalizedProjectPath = QDir::cleanPath(projectPath);
        if (normalizedProjectPath == vaultPath || normalizedProjectPath.startsWith(vaultPath + "/")) {
            preferredOpenPath = vault.path;
            break;
        }
    }

    if (!resolveObsidianOpenPath(preferredOpenPath).isEmpty()) {
        openVaultInObsidian(preferredOpenPath);
    }
}

void MainWindow::refreshMonitorAsync()
{
    if (m_monitorRefreshing) return;
    if (!m_monitorTabContent) return;

    m_monitorRefreshing = true;

    QFuture<QList<ProjectMonitorData>> future = QtConcurrent::run([this]() {
        QList<QPair<QString, QString>> projects = collectAllProjectPaths();
        SessionScanner scanner;
        return scanner.scanLiveSessions(projects);
    });
    m_monitorScanWatcher->setFuture(future);
}

void MainWindow::replaceMonitorContent(QWidget *newContent, bool preserveCurrentTab)
{
    if (!newContent) {
        return;
    }

    m_cachedMonitorWidget = newContent;

    if (!m_monitorTabContent) {
        m_monitorTabContent = newContent;
        return;
    }

    const int idx = m_mainTabWidget->indexOf(m_monitorTabContent);
    if (idx < 0) {
        if (m_monitorTabContent != newContent) {
            m_monitorTabContent->deleteLater();
        }
        m_monitorTabContent = newContent;
        return;
    }

    const bool wasCurrent = (m_mainTabWidget->currentIndex() == idx);
    m_mainTabWidget->removeTab(idx);
    if (m_monitorTabContent != newContent) {
        m_monitorTabContent->deleteLater();
    }
    m_monitorTabContent = newContent;

    const int newIdx = m_mainTabWidget->insertTab(idx, m_monitorTabContent, tr("Monitor Board"));
    addMonitorCloseButton(newIdx);
    if (preserveCurrentTab && wasCurrent) {
        m_mainTabWidget->setCurrentIndex(newIdx);
    }
    QTimer::singleShot(0, this, [this]() {
        updateSpecialMonitorPanelSizing();
    });
}

QString MainWindow::monitorRowKey(const QString& projectPath, const SessionInfo& session) const
{
    const QString stableSessionKey = session.sessionId.isEmpty() || session.sessionId == "unknown"
        ? QString("pid-%1").arg(session.processPid)
        : session.sessionId;
    const qint64 stableShellKey = session.shellPid > 0 ? session.shellPid : session.processPid;
    return QString("%1|%2|%3|%4")
        .arg(projectPath, session.toolName, stableSessionKey, QString::number(stableShellKey));
}

void MainWindow::updateMonitorStatusLabel(QLabel *label, SessionStatus status) const
{
    if (!label) {
        return;
    }

    QString statusText;
    QString statusColor;
    if (status == SessionStatus::Working) {
        statusColor = "#34c759";
        statusText = tr("Working");
    } else {
        statusColor = "#007aff";
        statusText = tr("Ready");
    }

    label->setText(QString("<span style='color: %1; font-size: 13px;'>&#9679;</span> <span style='font-size: 12px;'>%2</span>")
                       .arg(statusColor, statusText));
    label->setToolTip(statusText);
}

bool MainWindow::updateMonitorStatusCells(const QList<ProjectMonitorData>& data)
{
    if (!m_monitorTabContent) {
        return false;
    }

    QTreeWidget *tree = m_monitorTabContent->findChild<QTreeWidget*>("monitorBoardTree");
    if (!tree) {
        return false;
    }

    QList<QPair<QString, SessionStatus>> expectedRows;
    for (const ProjectMonitorData& pmd : data) {
        for (const SessionInfo& si : pmd.sessions) {
            expectedRows.append(qMakePair(monitorRowKey(pmd.projectPath, si), si.status));
        }
    }

    if (tree->topLevelItemCount() != expectedRows.size()) {
        return false;
    }

    for (int i = 0; i < expectedRows.size(); ++i) {
        QTreeWidgetItem *row = tree->topLevelItem(i);
        if (!row) {
            return false;
        }
        if (row->data(0, Qt::UserRole + 10).toString() != expectedRows[i].first) {
            return false;
        }
    }

    for (int i = 0; i < expectedRows.size(); ++i) {
        QTreeWidgetItem *row = tree->topLevelItem(i);
        if (!row) {
            continue;
        }

        const SessionStatus status = expectedRows[i].second;
        row->setData(0, Qt::UserRole + 6, static_cast<int>(status));
        QLabel *statusLabel = qobject_cast<QLabel*>(tree->itemWidget(row, 4));
        updateMonitorStatusLabel(statusLabel, status);
    }

    return true;
}

QWidget* MainWindow::buildMonitorView(const QList<ProjectMonitorData>& monitorData)
{
    QWidget *container = new QWidget();
    container->setStyleSheet("background: white;");
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(8);

    QList<Vault> vaults = m_vaultModel->vaults();
    if (vaults.isEmpty()) {
        QLabel *emptyLabel = new QLabel(tr("No vaults added. Add a vault to see monitor board."));
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("QLabel { color: #86868b; font-size: 15px; padding: 40px; background: white; }");
        mainLayout->addWidget(emptyLabel);
        return container;
    }

    // Legend + Refresh
    QHBoxLayout *legendLayout = new QHBoxLayout();

    QLabel *workingLegend = new QLabel();
    workingLegend->setStyleSheet("QLabel { background: transparent; border: none; }");
    workingLegend->setText(QString("<span style='color: #34c759; font-size: 14px;'>&#9679;</span> <span style='color: #86868b; font-size: 11px;'>%1</span>").arg(tr("Working")));
    legendLayout->addWidget(workingLegend);

    QLabel *readyLegend = new QLabel();
    readyLegend->setStyleSheet("QLabel { background: transparent; border: none; }");
    readyLegend->setText(QString("<span style='color: #007aff; font-size: 14px;'>&#9679;</span> <span style='color: #86868b; font-size: 11px;'>%1</span>").arg(tr("Ready")));
    legendLayout->addWidget(readyLegend);

    legendLayout->addStretch();

    QPushButton *refreshBtn = new QPushButton(tr("Refresh"));
    refreshBtn->setFixedSize(QSize(80, 24));
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet("QPushButton { background: #f5f5f7; color: #86868b; font-size: 11px; border: 1px solid #e0e0e0; border-radius: 4px; padding: 2px 8px; } QPushButton:hover { background: #e8e8ed; color: #333; }");
    refreshBtn->setProperty("isMonitorRefreshBtn", true);
    legendLayout->addWidget(refreshBtn);

    mainLayout->addLayout(legendLayout);

    // Flat table - no tree nesting
    QTreeWidget *tree = new QTreeWidget();
    tree->setObjectName("monitorBoardTree");
    tree->setHeaderLabels(QStringList()
        << tr("Project") << tr("Terminal") << tr("AI Tool")
        << tr("Session") << tr("Status") << tr("Action"));
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->setSelectionMode(QAbstractItemView::NoSelection);
    tree->setTextElideMode(Qt::ElideMiddle);
    tree->setProperty("isMonitorTree", true);
    tree->installEventFilter(this);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tree->header()->setStretchLastSection(false);
    tree->header()->setMinimumSectionSize(72);
    tree->header()->setSectionsClickable(true);
    tree->header()->setSectionResizeMode(0, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(3, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(4, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(5, QHeaderView::Fixed);
    tree->setStyleSheet(
        "QTreeWidget { border: 1px solid #e8e8ed; border-radius: 6px; background: white; }"
        "QTreeWidget::item { padding: 6px 10px; border-bottom: 1px solid #f5f5f7; }"
        "QTreeWidget::item:selected { background: #e8f4fd; color: #1d1d1f; }"
        "QHeaderView::section { background: #f5f5f7; color: #86868b; font-size: 12px; font-weight: 600; padding: 6px 4px; border: none; border-bottom: 1px solid #e0e0e0; }"
    );
    connect(tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int) {
                openMonitorTarget(item);
            });

    if (monitorData.isEmpty()) {
        QTreeWidgetItem *emptyItem = new QTreeWidgetItem(tree);
        emptyItem->setText(0, tr("No active AI sessions detected."));
        emptyItem->setForeground(0, QColor("#86868b"));
        emptyItem->setFirstColumnSpanned(true);
    } else {
        QString lastProjectName;
        for (const ProjectMonitorData& pmd : monitorData) {
            if (pmd.sessions.isEmpty()) {
                continue;
            }

            for (const SessionInfo& si : pmd.sessions) {
                QTreeWidgetItem *row = new QTreeWidgetItem(tree);

                // Col 0: Project (dedup - only show once)
                if (pmd.projectName == lastProjectName) {
                    row->setText(0, QString());
                } else {
                    row->setText(0, pmd.projectName);
                    lastProjectName = pmd.projectName;
                }
                row->setToolTip(0, pmd.projectPath);

                // Col 1: Terminal
                row->setText(1, si.terminalName);
                row->setToolTip(1, si.terminalName);
                if (!si.terminalIconPath.isEmpty()) {
                    QPixmap terminalPix(si.terminalIconPath);
                    if (!terminalPix.isNull()) {
                        row->setIcon(1, QIcon(terminalPix));
                    }
                }

                // Col 2: AI Tool with icon
                row->setText(2, si.toolName);
                row->setToolTip(2, si.toolName);
                if (!si.toolIconPath.isEmpty()) {
                    QPixmap pix(si.toolIconPath);
                    if (!pix.isNull()) {
                        row->setIcon(2, QIcon(pix.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                    }
                }

                // Col 3: Session ID (from tool config, not PID)
                QString displaySessionId = si.sessionId;
                row->setText(3, displaySessionId);
                row->setToolTip(3, QString("Session: %1\nTool: %2\nPath: %3").arg(si.sessionId, si.toolName, si.projectPath));

                // Col 4: Status
                QLabel *statusLabel = new QLabel();
                statusLabel->setAlignment(Qt::AlignCenter);
                statusLabel->setStyleSheet("QLabel { background: transparent; border: none; padding: 0 4px; }");
                updateMonitorStatusLabel(statusLabel, si.status);
                tree->setItemWidget(row, 4, statusLabel);

                // Col 5: Goto button - activate terminal directly
                QPushButton *gotoBtn = new QPushButton(tr("Goto"));
                gotoBtn->setMinimumSize(QSize(72, 28));
                gotoBtn->setCursor(Qt::PointingHandCursor);
                gotoBtn->setStyleSheet(
                    "QPushButton { background: #34c759; color: white; font-size: 11px; font-weight: 500; border: none; border-radius: 4px; }"
                    "QPushButton:hover { background: #2db85e; }");
                tree->setItemWidget(row, 5, gotoBtn);

                row->setData(0, Qt::UserRole, pmd.projectPath);
                row->setData(0, Qt::UserRole + 1, si.terminalName);
                row->setData(0, Qt::UserRole + 2, si.terminalPid);
                row->setData(0, Qt::UserRole + 3, si.toolName);
                row->setData(0, Qt::UserRole + 4, si.detailText);
                row->setData(0, Qt::UserRole + 5, si.sessionPath);
                row->setData(0, Qt::UserRole + 6, static_cast<int>(si.status));
                row->setData(0, Qt::UserRole + 7, pmd.projectName);
                row->setData(0, Qt::UserRole + 8, si.processPid);
                row->setData(0, Qt::UserRole + 9, si.sessionId);
                row->setData(0, Qt::UserRole + 10, monitorRowKey(pmd.projectPath, si));

                connect(gotoBtn, &QPushButton::clicked, this, [this, row]() {
                    openMonitorTarget(row);
                });
            }
        }
    }

    m_monitorVerticalSplitter = new QSplitter(Qt::Vertical, container);
    m_monitorVerticalSplitter->setChildrenCollapsible(false);
    m_monitorVerticalSplitter->setHandleWidth(10);
    m_monitorVerticalSplitter->setStyleSheet(
        "QSplitter::handle:vertical { background: #e8e8ed; height: 10px; margin: 2px 0; border-radius: 4px; }"
        "QSplitter::handle:vertical:hover { background: #d0d0d7; }");
    m_specialMonitorPanelUserResized = false;

    m_monitorVerticalSplitter->addWidget(tree);
    m_specialMonitorPanel = buildSpecialMonitorPanel();
    m_monitorVerticalSplitter->addWidget(m_specialMonitorPanel);
    connect(m_monitorVerticalSplitter, &QSplitter::splitterMoved, this, [this]() {
        m_specialMonitorPanelUserResized = true;
    });
    mainLayout->addWidget(m_monitorVerticalSplitter, 1);

    QPushButton *btn = container->findChild<QPushButton*>("", Qt::FindDirectChildrenOnly);
    if (btn && btn->property("isMonitorRefreshBtn").toBool()) {
        connect(btn, &QPushButton::clicked, this, &MainWindow::refreshMonitorAsync);
    }

    updateMonitorTableColumns(tree);
    QTimer::singleShot(0, tree, [this, tree]() {
        updateMonitorTableColumns(tree);
    });
    QTimer::singleShot(0, tree, [this, tree]() {
        for (int col = 0; col < tree->columnCount(); ++col) {
            tree->header()->setSectionResizeMode(col, QHeaderView::Interactive);
        }
        const QByteArray headerState = m_settingsManager->monitorBoardHeaderState();
        if (!headerState.isEmpty()) {
            tree->header()->restoreState(headerState);
        }
        connect(tree->header(), &QHeaderView::sectionResized, this, [this, tree]() {
            m_settingsManager->setMonitorBoardHeaderState(tree->header()->saveState());
            m_settingsManager->sync();
        });
    });
    QTimer::singleShot(0, this, [this]() {
        updateSpecialMonitorPanelSizing();
    });
    QTimer::singleShot(0, this, [this]() {
        refreshSpecialMonitorAsync();
    });

    return container;
}

void MainWindow::updateMonitorTableColumns(QTreeWidget *tree)
{
    if (!tree || tree->columnCount() < 6) {
        return;
    }

    const int viewportWidth = tree->viewport()->width();
    if (viewportWidth <= 0) {
        return;
    }

    int projectWidth = qBound(150, viewportWidth / 8, 240);
    int terminalWidth = viewportWidth < 820 ? 108 : (viewportWidth < 1120 ? 124 : 144);
    int toolWidth = viewportWidth < 820 ? 136 : (viewportWidth < 1120 ? 168 : 196);
    int statusWidth = viewportWidth < 820 ? 104 : 124;
    int actionWidth = viewportWidth < 820 ? 104 : 128;
    const int minProjectWidth = 112;
    const int minTerminalWidth = 96;
    const int minToolWidth = 128;
    const int minStatusWidth = 96;
    const int minActionWidth = 96;
    const int minSessionWidth = 180;
    const int chromeWidth = 6;

    auto sessionWidth = [&]() {
        return viewportWidth - (projectWidth + terminalWidth + toolWidth + statusWidth + actionWidth + chromeWidth);
    };

    auto shrinkToFit = [&](int &value, int minValue) {
        const int deficit = minSessionWidth - sessionWidth();
        if (deficit <= 0 || value <= minValue) {
            return;
        }
        const int delta = qMin(deficit, value - minValue);
        value -= delta;
    };

    shrinkToFit(projectWidth, minProjectWidth);
    shrinkToFit(terminalWidth, minTerminalWidth);
    shrinkToFit(toolWidth, minToolWidth);
    shrinkToFit(statusWidth, minStatusWidth);
    shrinkToFit(actionWidth, minActionWidth);

    int finalSessionWidth = qMax(minSessionWidth, sessionWidth());
    const int extraWidth = viewportWidth - (projectWidth + terminalWidth + toolWidth + finalSessionWidth + statusWidth + actionWidth + chromeWidth);
    if (extraWidth > 0) {
        finalSessionWidth += (extraWidth * 2) / 3;
        projectWidth += extraWidth / 3;
    }

    tree->setColumnWidth(0, projectWidth);
    tree->setColumnWidth(1, terminalWidth);
    tree->setColumnWidth(2, toolWidth);
    tree->setColumnWidth(3, finalSessionWidth);
    tree->setColumnWidth(4, statusWidth);
    tree->setColumnWidth(5, actionWidth);
}

bool MainWindow::activateTerminalWindow(qint64 pid)
{
#ifdef Q_OS_MAC
    if (pid <= 0) {
        return false;
    }

    QString commandPath;
    {
        QProcess ps;
        ps.start("ps", QStringList() << "-p" << QString::number(pid) << "-o" << "comm=");
        if (ps.waitForFinished(2000) && ps.exitStatus() == QProcess::NormalExit && ps.exitCode() == 0) {
            commandPath = QString::fromUtf8(ps.readAllStandardOutput()).trimmed();
        }
    }

    QString appName;
    const QString normalizedCommand = commandPath.trimmed();
    if (normalizedCommand.contains(".app/Contents/MacOS/")) {
        QRegularExpression appRegex(R"(([^/]+)\.app/Contents/MacOS/[^/]+$)");
        const QRegularExpressionMatch match = appRegex.match(normalizedCommand);
        if (match.hasMatch()) {
            appName = match.captured(1);
        }
    }

    if (appName.isEmpty() && !normalizedCommand.isEmpty()) {
        const QString baseName = QFileInfo(normalizedCommand).fileName();
        const QString lower = baseName.toLower();
        if (lower == "ghostty") {
            appName = "Ghostty";
        } else if (lower == "wezterm-gui" || lower == "wezterm") {
            appName = "WezTerm";
        } else if (lower == "iterm2" || lower == "iterm") {
            appName = "iTerm";
        } else if (lower == "terminal") {
            appName = "Terminal";
        }
    }

    QString script = QString(
        "set targetPid to %1\n"
        "set targetApp to %2\n"
        "if targetApp is not \"\" then\n"
        "    tell application targetApp to activate\n"
        "end if\n"
        "tell application \"System Events\"\n"
        "    set procList to every process whose unix id is targetPid\n"
        "    if (count of procList) > 0 then\n"
        "        tell item 1 of procList\n"
        "            set frontmost to true\n"
        "            try\n"
        "                if (count of windows) > 0 then\n"
        "                    perform action \"AXRaise\" of window 1\n"
        "                end if\n"
        "            end try\n"
        "        end tell\n"
        "        return \"ok\"\n"
        "    end if\n"
        "end tell"
    ).arg(pid).arg(appName.isEmpty() ? QString("\"\"") : QString("\"%1\"").arg(appName));

    QProcess osascript;
    osascript.start("osascript", QStringList() << "-e" << script);
    if (!osascript.waitForFinished(3000)) {
        osascript.kill();
        return false;
    }

    const QString output = QString::fromUtf8(osascript.readAllStandardOutput()).trimmed();
    return osascript.exitStatus() == QProcess::NormalExit &&
           osascript.exitCode() == 0 &&
           output == "ok";
#else
    Q_UNUSED(pid);
    return false;
#endif
}

void MainWindow::showSessionDetailDialog(const SessionInfo& session)
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Session Detail - %1").arg(session.toolName));
    dialog.setMinimumSize(600, 400);
    dialog.resize(700, 500);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    QLabel *headerLabel = new QLabel(QString("<h3>%1</h3>").arg(session.toolName));
    headerLabel->setStyleSheet("QLabel { color: #1d1d1f; }");
    layout->addWidget(headerLabel);
    
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(8);
    
    auto addRow = [&](int row, const QString& label, const QString& value) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("QLabel { color: #86868b; font-size: 13px; }");
        grid->addWidget(lbl, row, 0);
        QLabel *val = new QLabel(value);
        val->setWordWrap(true);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        val->setStyleSheet("QLabel { color: #1d1d1f; font-size: 13px; }");
        grid->addWidget(val, row, 1);
    };
    
    addRow(0, tr("Tool"), session.toolName);
    addRow(1, tr("Project"), session.projectName);
    addRow(2, tr("Project Path"), session.projectPath);
    
    QString statusText;
    if (session.status == SessionStatus::Working) {
        statusText = tr("Working");
    } else if (session.status == SessionStatus::Ready) {
        statusText = tr("Ready");
    } else {
        statusText = tr("Unknown");
    }
    addRow(3, tr("Status"), statusText);
    addRow(4, tr("Terminal"), session.terminalName);
    addRow(5, tr("Last Activity"), session.lastActivity.toString("yyyy-MM-dd HH:mm:ss"));
    addRow(6, tr("Session Path"), session.sessionPath);
    addRow(7, tr("Detail"), session.detailText);
    
    layout->addLayout(grid);
    
    layout->addStretch();
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *closeBtn = new QPushButton(tr("Close"), &dialog);
    closeBtn->setFixedWidth(80);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
    
    dialog.setStyleSheet(ThemeManager::instance()->currentStyle());
    dialog.exec();
}
