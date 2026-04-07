#include "mainwindow.h"
#include "theme/thememanager.h"
#include "models/vaultmodel.h"
#include "utils/settingsmanager.h"
#include "utils/vaultvalidator.h"
#include "utils/obsidianconfigreader.h"
#include "utils/r2moscanner.h"
#include "utils/aitoolscanner.h"
#include "utils/gitscanner.h"
#include "i18n/translationmanager.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
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
#include <QTabWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QHeaderView>
#include <QSpacerItem>
#include <QStatusBar>
#include <QPixmap>

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
    
    // Swimlane button
    m_swimlaneBtn = new QPushButton();
    m_swimlaneBtn->setText("📊");
    m_swimlaneBtn->setObjectName("swimlaneBtn");
    m_swimlaneBtn->setCursor(Qt::PointingHandCursor);
    m_swimlaneBtn->setToolTip(tr("Swimlane View"));
    leftLayout->addWidget(m_swimlaneBtn);
    
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
    m_vaultList->setSpacing(4);
    m_vaultList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_vaultList->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_vaultList->setStyleSheet("QListWidget::item { min-height: 36px; padding: 8px 12px; border-radius: 3px; } QListWidget::item:selected { background: #007aff; color: white; }");

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
    m_vaultStatsHeader = new QLabel(tr("📊 Vault Statistics"));
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
    m_r2moStatsHeader = new QLabel(tr("🔧 R2MO Statistics"));
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
    m_mainTabWidget->setTabsClosable(true);
    m_mainTabWidget->setStyleSheet(
        "QTabWidget#mainTabs { background: transparent; border: none; }"
        "QTabWidget#mainTabs::pane { border: none; background: transparent; }"
        "QTabWidget#mainTabs::tab-bar { alignment: left; }"
        "QTabWidget#mainTabs QTabBar::tab { padding: 6px 16px; margin-right: 2px; background: #f0f0f0; border: 1px solid #d0d0d0; border-bottom: none; border-radius: 4px 4px 0 0; }"
        "QTabWidget#mainTabs QTabBar::tab:selected { background: white; color: #007aff; border-color: #007aff; }"
        "QTabWidget#mainTabs QTabBar::tab:!selected { background: #e8e8e8; color: #666; }"
        "QTabBar::close-button { subcontrol-position: right; margin: 2px; }"
    );
    m_homeTabContent = homeTabContent;
    m_mainTabWidget->addTab(m_homeTabContent, tr("🏠 HOME"));
    // HOME tab is not closable
    m_mainTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    
    // No swimlane tab initially — opened by button, closable
    m_swimlaneTabContent = nullptr;
    
    mainLayout->addWidget(m_mainTabWidget, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupConnections()
{
    connect(m_vaultList, &QListWidget::itemClicked, this, &MainWindow::onVaultSelected);
    connect(m_vaultList, &QListWidget::itemDoubleClicked, this, &MainWindow::onVaultDoubleClicked);
    connect(m_vaultList, &QListWidget::customContextMenuRequested, this, &MainWindow::onVaultContextMenu);
    connect(m_vaultList, &QListWidget::itemChanged, this, &MainWindow::onVaultItemChanged);
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
    // Keep preview inner tabs fixed (non-closeable)
    // Main tab close: allow closing swimlane tab (index > 0)
    connect(m_mainTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index > 0) {
            QWidget *w = m_mainTabWidget->widget(index);
            m_mainTabWidget->removeTab(index);
            if (w == m_swimlaneTabContent) {
                m_swimlaneTabContent->deleteLater();
                m_swimlaneTabContent = nullptr;
                if (m_swimlaneRefreshTimer) {
                    m_swimlaneRefreshTimer->stop();
                }
            }
        }
    });
    
    // Swimlane refresh timer (30 seconds)
    m_swimlaneRefreshTimer = new QTimer(this);
    m_swimlaneRefreshTimer->setInterval(30000);
    connect(m_swimlaneRefreshTimer, &QTimer::timeout, this, &MainWindow::onSwimlaneRefresh);
}

void MainWindow::updateVaultList()
{
    m_vaultList->clear();
    
    QList<Vault> vaults = m_vaultModel->vaults();
    for (const Vault& v : vaults) {
        QListWidgetItem *item = new QListWidgetItem(v.name);
        item->setData(Qt::UserRole, v.path);
        item->setToolTip(v.path);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_vaultList->addItem(item);
    }
}

void MainWindow::onAddVault()
{
    // Create selection dialog with tree view
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Obsidian Vault"));
    dialog.setMinimumSize(1200, 600);
    dialog.resize(1400, 700);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    // Header
    QLabel* headerLabel = new QLabel(tr("Select an Obsidian Vault:"));
    headerLabel->setStyleSheet("font-size: 15px; font-weight: 600;");
    layout->addWidget(headerLabel);
    
    // Tree widget
    QTreeWidget* treeWidget = new QTreeWidget();
    treeWidget->setHeaderLabels(QStringList() << tr("Name") << tr("Status") << tr("Path"));
    treeWidget->setRootIsDecorated(true);
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeWidget->header()->setStretchLastSection(true);
    treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    
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
            
            // Check if it's a valid Obsidian vault
            QDir subDir(fullPath);
            bool isVault = subDir.exists(".obsidian");
            
            if (isVault) {
                // Valid vault - can be selected
                QString vaultName = m_vaultValidator->getVaultName(fullPath);
                
                // Check if already added
                bool alreadyAdded = m_vaultModel->contains(fullPath);
                
                // Column 0: Name with icon
                item->setText(0, "📁 " + subdir);
                
                // Column 1: Status with color
                if (alreadyAdded) {
                    item->setText(1, "[Added]");
                    item->setForeground(1, QColor("#86868b"));
                } else if (openVaults.contains(fullPath)) {
                    item->setText(1, "● Open");
                    item->setForeground(1, QColor("#34c759"));
                } else {
                    item->setText(1, "Vault");
                    item->setForeground(1, QColor("#007aff"));
                }
                
                // Column 2: Full path
                item->setText(2, fullPath);
                
                item->setData(0, Qt::UserRole + 1, vaultName);
                item->setData(0, Qt::UserRole + 2, !alreadyAdded);  // Can select if not added
                
            } else {
                // Not a vault - gray out but can expand
                item->setText(0, "📂 " + subdir);
                item->setText(1, "");
                item->setText(2, fullPath);
                item->setForeground(0, QColor("#86868b"));
                item->setForeground(2, QColor("#86868b"));
                item->setData(0, Qt::UserRole + 2, false);  // Not a vault
                
                // Add placeholder child to show expand arrow
                QTreeWidgetItem* placeholder = new QTreeWidgetItem(item);
                placeholder->setText(0, tr("Loading..."));
                placeholder->setData(0, Qt::UserRole, "");
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
    homeItem->setData(0, Qt::UserRole + 2, false);  // Not a vault
    homeItem->setForeground(0, QColor("#86868b"));
    homeItem->setForeground(2, QColor("#86868b"));
    
    // Add placeholder to show expand arrow
    QTreeWidgetItem* homePlaceholder = new QTreeWidgetItem(homeItem);
    homePlaceholder->setText(0, tr("Loading..."));
    homePlaceholder->setData(0, Qt::UserRole, "");
    
    // Also add known vaults from Obsidian config as quick access
    QTreeWidgetItem* knownItem = new QTreeWidgetItem(treeWidget);
    knownItem->setText(0, "⭐ " + tr("Known Vaults (from Obsidian)"));
    knownItem->setText(1, "");
    knownItem->setText(2, "");
    knownItem->setData(0, Qt::UserRole, "");
    knownItem->setData(0, Qt::UserRole + 2, false);
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
        vaultItem->setData(0, Qt::UserRole + 2, true);  // Is vault
        
        vaultItem->setToolTip(0, info.path);
    }
    
    // If no known vaults, remove the section
    if (knownItem->childCount() == 0) {
        delete knownItem;
    }
    
    layout->addWidget(treeWidget);
    
    // Info label
    QLabel* infoLabel = new QLabel(tr("📁 = Vault (can be added)  |  📂 = Directory (expand to navigate)  |  ● Open = Currently open in Obsidian"));
    infoLabel->setStyleSheet("color: #86868b; font-size: 12px;");
    layout->addWidget(infoLabel);
    
    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton* addBtn = new QPushButton(tr("Add"));
    addBtn->setDefault(true);
    addBtn->setFixedWidth(80);
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
            }
        }
    });
    
    // Connect buttons
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(addBtn, &QPushButton::clicked, [&]() {
        QTreeWidgetItem* selected = treeWidget->currentItem();
        if (selected && selected->data(0, Qt::UserRole + 2).toBool()) {
            dialog.accept();
        } else {
            QMessageBox::information(&dialog, tr("Select Vault"), 
                tr("Please select a valid Obsidian vault (📁 items)."));
        }
    });
    connect(treeWidget, &QTreeWidget::itemDoubleClicked, [&](QTreeWidgetItem* item, int) {
        if (item && item->data(0, Qt::UserRole + 2).toBool()) {
            dialog.accept();
        }
    });
    
    // Expand home directory by default
    treeWidget->expandItem(homeItem);
    
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
            
            QString name = QInputDialog::getText(this, tr("Vault Name"), tr("Enter vault name:"),
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
    openVaultInObsidian(path);
}

void MainWindow::onVaultDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    QString path = item->data(Qt::UserRole).toString();
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
    
    QAction *openAction = contextMenu.addAction(tr("Open in Obsidian"));
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    
    contextMenu.addSeparator();
    
    QAction *removeAction = contextMenu.addAction(tr("Remove"));
    
    QAction *selectedAction = contextMenu.exec(m_vaultList->mapToGlobal(pos));
    
    if (selectedAction == renameAction) {
        m_editingVaultPath = item->data(Qt::UserRole).toString();
        m_vaultList->editItem(item);
    } else if (selectedAction == openAction) {
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

void MainWindow::openVaultInObsidian(const QString& vaultPath)
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

void MainWindow::updatePreviewPane(const QString& name, const QString& path)
{
    m_currentPreviewPath = path;
    
    // Update title and show buttons
    m_previewTitle->setText(name);
    m_previewTitle->show();
    m_previewEditBtn->show();
    m_previewOpenBtn->show();
    m_previewTitleEdit->hide();
    
    // Clear previous content
    m_graphScene->clear();
    m_taskTree->clear();
    if (path.trimmed().isEmpty()) {
        setOverviewEmptyState(true);
        return;
    }

    setOverviewEmptyState(false);
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
}

void MainWindow::onThemeToggle()
{
    ThemeManager::Theme current = ThemeManager::instance()->currentTheme();
    ThemeManager::Theme newTheme = (current == ThemeManager::Light) ? ThemeManager::Dark : ThemeManager::Light;
    ThemeManager::instance()->setTheme(newTheme);
    m_settingsManager->setTheme(static_cast<int>(newTheme));
}

void MainWindow::onSwimlane()
{
    // If swimlane tab already exists, just switch to it
    if (m_swimlaneTabContent) {
        int idx = m_mainTabWidget->indexOf(m_swimlaneTabContent);
        if (idx >= 0) {
            m_mainTabWidget->setCurrentIndex(idx);
            return;
        }
    }

    // Build and add swimlane tab
    m_mainTabWidget->setTabsClosable(true);
    m_swimlaneTabContent = buildSwimlaneView();
    int newIdx = m_mainTabWidget->addTab(m_swimlaneTabContent, tr("📊 泳道图"));
    // HOME tab remains non-closeable
    m_mainTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    m_mainTabWidget->setCurrentIndex(newIdx);
    
    // Start refresh timer
    if (m_swimlaneRefreshTimer) {
        m_swimlaneRefreshTimer->start();
    }
}

QWidget* MainWindow::buildSwimlaneView()
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

    // Legend
    QLabel *legendLabel = new QLabel(tr("🟠 Running Tasks"));
    legendLabel->setStyleSheet("QLabel { color: #86868b; font-size: 11px; background: white; }");
    mainLayout->addWidget(legendLabel);

    // Scrollable tree-style swimlane
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #e0e0e0; background: white; border-radius: 4px; }");

    QWidget *swimlaneContent = new QWidget();
    swimlaneContent->setStyleSheet("background: white;");
    QVBoxLayout *contentLayout = new QVBoxLayout(swimlaneContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Colors
    QColor borderColor("#e0e0e0");
    QColor textColor("#333333");
    QColor headerBg("#f5f5f7");
    QColor laneBg("#ffffff");
    QColor pendingColor("#ff9500");
    QColor emptyColor("#f0f0f0");

    // Collect max queue count across ALL vaults for uniform column width
    int globalMaxQueue = 0;
    struct VaultSwimlaneData {
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
    QList<VaultSwimlaneData> allVaultData;

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

        VaultSwimlaneData vd;
        vd.vaultName = vault.name;
        vd.vaultPath = vault.path;
        
        GitScanner gitScanner;
        vd.gitStatus = gitScanner.scanRepository(vault.path);
        
        AIToolScanner aiScanner;
        vd.aiTools = aiScanner.scanProject(vault.path);

        // Parent row
        VaultSwimlaneData::LaneRow parentRow;
        parentRow.name = parent->name;
        parentRow.projectPath = parent->path;
        parentRow.isParent = true;
        parentRow.r2moPath = parent->path + "/.r2mo";
        parentRow.historicalCount = scanner.getHistoricalTasks(parent->path + "/.r2mo").size();
        parentRow.queueTasks = scanner.getTaskQueueFiles(parent->path + "/.r2mo");
        parentRow.gitStatus = gitScanner.scanRepository(parent->path);
        if (parentRow.queueTasks.size() > globalMaxQueue)
            globalMaxQueue = parentRow.queueTasks.size();

        // Children
        for (const R2moSubProject* child : children) {
            VaultSwimlaneData::LaneRow childRow;
            childRow.name = child->name;
            childRow.projectPath = child->path;
            childRow.isParent = false;
            childRow.r2moPath = child->path + "/.r2mo";
            childRow.historicalCount = scanner.getHistoricalTasks(child->path + "/.r2mo").size();
            childRow.queueTasks = scanner.getTaskQueueFiles(child->path + "/.r2mo");
            childRow.gitStatus = gitScanner.scanRepository(child->path);
            if (childRow.queueTasks.size() > globalMaxQueue)
                globalMaxQueue = childRow.queueTasks.size();
            parentRow.children.append(childRow);
        }
        vd.rows.append(parentRow);
        allVaultData.append(vd);
    }

    if (allVaultData.isEmpty()) {
        QLabel *noDataLabel = new QLabel(tr("No R2MO projects found in any vault."));
        noDataLabel->setAlignment(Qt::AlignCenter);
        noDataLabel->setStyleSheet("QLabel { color: #86868b; font-size: 15px; padding: 40px; background: white; }");
        contentLayout->addWidget(noDataLabel);
        scrollArea->setWidget(swimlaneContent);
        mainLayout->addWidget(scrollArea, 1);
        return container;
    }

    int nameColWidth = 260;
    int histColWidth = 40;
    int cellWidth = 36;
    int cellHeight = 24;
    int rowHeight = 28;
    int gridColCount = qMax(1, globalMaxQueue);

    // Colors
    QColor runningColor("#007aff");   // Blue for running tasks
    QColor emptyBorder("#d0d0d0");    // Gray border for empty slots

    // Helper lambda to add one lane row
    auto addLaneRow = [&](QGridLayout *grid, int row, const QString &name,
                          int histCount, const QList<TaskInfo> &queueTasks,
                          const QString &r2moPath, bool isChild,
                          const GitStatusInfo &gitStatus) {
        Q_UNUSED(gitStatus);
        
        // Name label — fixed width for alignment, red color only
        QString nameText = isChild ? QString("  └ %1").arg(name) : name;
        QLabel *nameLabel = new QLabel(nameText);
        nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameLabel->setFixedSize(nameColWidth, rowHeight);
        nameLabel->setStyleSheet(QString("QLabel { color: #ff3b30; font-size: 14px; padding: 2px 4px; background: %1; border-bottom: 1px solid %2; }")
            .arg(laneBg.name()).arg(borderColor.name()));
        if (!isChild) {
            QFont f = nameLabel->font();
            f.setBold(true);
            nameLabel->setFont(f);
        }
        grid->addWidget(nameLabel, row, 0);

        // Historical count — only show if > 0, clickable
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

        // Queue cells — running tasks in blue, empty slots with gray border
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

    // Build rows for each vault
    for (const VaultSwimlaneData &vd : allVaultData) {
        // Vault header with Git status and AI info (no border)
        QWidget *vaultHeaderWidget = new QWidget();
        QHBoxLayout *vaultHeaderLayout = new QHBoxLayout(vaultHeaderWidget);
        vaultHeaderLayout->setContentsMargins(8, 4, 8, 4);
        vaultHeaderLayout->setSpacing(8);
        vaultHeaderWidget->setStyleSheet(QString("QWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #e8f4ff, stop:0.5 #f0e8ff, stop:1 #fff8f0); border: none; border-radius: 6px; }"));
        
        // Left side: Vault name (larger font)
        QLabel *vaultNameLabel = new QLabel(QString("📁 %1").arg(vd.vaultName));
        vaultNameLabel->setStyleSheet("QLabel { color: #af52de; font-size: 14px; font-weight: 600; background: transparent; border: none; }");
        vaultHeaderLayout->addWidget(vaultNameLabel);
        
        // AI Tools info - each tool with icon, name, and session count
        for (const AIToolInfo& tool : vd.aiTools) {
            QString iconPath;
            QString displayName = tool.name;
            if (tool.name.contains("Claude", Qt::CaseInsensitive)) iconPath = ":/ai-tools/claude.png";
            else if (tool.name.contains("Codex", Qt::CaseInsensitive)) iconPath = ":/ai-tools/codex.png";
            else if (tool.name.contains("OpenCode", Qt::CaseInsensitive)) iconPath = ":/ai-tools/opencode.png";
            else if (tool.name.contains("Cursor", Qt::CaseInsensitive)) iconPath = ":/ai-tools/cursor.png";
            else if (tool.name.contains("Cherry", Qt::CaseInsensitive)) iconPath = ":/ai-tools/cherry.png";
            
            QWidget *toolWidget = new QWidget();
            QHBoxLayout *toolLayout = new QHBoxLayout(toolWidget);
            toolLayout->setContentsMargins(0, 0, 0, 0);
            toolLayout->setSpacing(4);
            
            // Try to load icon
            bool hasIcon = false;
            if (!iconPath.isEmpty()) {
                QPixmap pixmap(iconPath);
                hasIcon = !pixmap.isNull();
                if (hasIcon) {
                    QLabel *iconLabel = new QLabel();
                    iconLabel->setPixmap(pixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    iconLabel->setStyleSheet("QLabel { background: transparent; border: none; }");
                    toolLayout->addWidget(iconLabel);
                }
            }
            
            // Always show tool name
            QLabel *nameLabel = new QLabel(displayName);
            nameLabel->setStyleSheet("QLabel { color: #1d1d1f; font-size: 14px; background: transparent; border: none; font-weight: 500; }");
            toolLayout->addWidget(nameLabel);
            
            // Session count
            QLabel *countLabel = new QLabel(QString("(%1)").arg(tool.sessions.count));
            countLabel->setStyleSheet("QLabel { color: #86868b; font-size: 12px; background: transparent; border: none; }");
            toolLayout->addWidget(countLabel);
            
            vaultHeaderLayout->addWidget(toolWidget);
        }
        
        vaultHeaderLayout->addStretch();
        
        // Right side: Git status
        if (vd.gitStatus.isGitRepo) {
            QWidget *gitWidget = new QWidget();
            QHBoxLayout *gitLayout = new QHBoxLayout(gitWidget);
            gitLayout->setContentsMargins(0, 0, 0, 0);
            gitLayout->setSpacing(6);
            
            // Check if has any changes (need star marker)
            bool hasChanges = (vd.gitStatus.aheadCount > 0 || vd.gitStatus.behindCount > 0 ||
                              vd.gitStatus.stagedCount > 0 || vd.gitStatus.modifiedCount > 0 || 
                              vd.gitStatus.untrackedCount > 0);
            
            // Branch name with optional star marker
            QString branchText = hasChanges ? QString("★ %1").arg(vd.gitStatus.branch) : vd.gitStatus.branch;
            QLabel *branchLabel = new QLabel(branchText);
            branchLabel->setStyleSheet("QLabel { color: #b8860b; font-size: 14px; background: transparent; border: none; font-weight: 600; }");
            gitLayout->addWidget(branchLabel);
            
            // Only show markers if there are changes
            if (hasChanges) {
                QString markerStyle = "QLabel { font-size: 14px; background: transparent; border: none; font-weight: 600; }";
                
                if (vd.gitStatus.aheadCount > 0) {
                    QLabel *aheadLabel = new QLabel(QString("↑%1").arg(vd.gitStatus.aheadCount));
                    aheadLabel->setStyleSheet(QString("%1 color: #007aff; min-width: 28px; }").arg(markerStyle));
                    gitLayout->addWidget(aheadLabel);
                }
                
                if (vd.gitStatus.behindCount > 0) {
                    QLabel *behindLabel = new QLabel(QString("↓%1").arg(vd.gitStatus.behindCount));
                    behindLabel->setStyleSheet(QString("%1 color: #ff9500; min-width: 28px; }").arg(markerStyle));
                    gitLayout->addWidget(behindLabel);
                }
                
                if (vd.gitStatus.stagedCount > 0) {
                    QLabel *stagedLabel = new QLabel(QString("!%1").arg(vd.gitStatus.stagedCount));
                    stagedLabel->setStyleSheet(QString("%1 color: #34c759; min-width: 24px; }").arg(markerStyle));
                    gitLayout->addWidget(stagedLabel);
                }
                
                if (vd.gitStatus.modifiedCount > 0) {
                    QLabel *modifiedLabel = new QLabel(QString("✗%1").arg(vd.gitStatus.modifiedCount));
                    modifiedLabel->setStyleSheet(QString("%1 color: #ff3b30; min-width: 24px; }").arg(markerStyle));
                    gitLayout->addWidget(modifiedLabel);
                }
                
                if (vd.gitStatus.untrackedCount > 0) {
                    QLabel *untrackedLabel = new QLabel(QString("?%1").arg(vd.gitStatus.untrackedCount));
                    untrackedLabel->setStyleSheet(QString("%1 color: #86868b; min-width: 24px; }").arg(markerStyle));
                    gitLayout->addWidget(untrackedLabel);
                }
            }
            
            vaultHeaderLayout->addWidget(gitWidget);
        }
        
        contentLayout->addWidget(vaultHeaderWidget);

        QWidget *gridWidget = new QWidget();
        gridWidget->setStyleSheet("background: white;");
        QGridLayout *grid = new QGridLayout(gridWidget);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(0);

        // Column headers
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

        // Spacer column to push everything left
        grid->setColumnStretch(gridColCount + 2, 1);

        int rowIdx = 1;
        for (const VaultSwimlaneData::LaneRow &parentRow : vd.rows) {
            // Parent project row
            addLaneRow(grid, rowIdx++, parentRow.name, parentRow.historicalCount, parentRow.queueTasks, parentRow.r2moPath, false, parentRow.gitStatus);
            // Child rows
            for (const VaultSwimlaneData::LaneRow &childRow : parentRow.children) {
                addLaneRow(grid, rowIdx++, childRow.name, childRow.historicalCount, childRow.queueTasks, childRow.r2moPath, true, childRow.gitStatus);
            }
        }

        contentLayout->addWidget(gridWidget);
        contentLayout->addSpacing(12);
    }

    contentLayout->addStretch(1);
    scrollArea->setWidget(swimlaneContent);
    mainLayout->addWidget(scrollArea, 1);

    return container;
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
    // Only refresh if swimlane tab is visible
    if (!m_swimlaneTabContent) {
        return;
    }
    
    int idx = m_mainTabWidget->indexOf(m_swimlaneTabContent);
    if (idx < 0 || m_mainTabWidget->currentIndex() != idx) {
        return;
    }
    
    // Rebuild swimlane view
    QWidget *newWidget = buildSwimlaneView();
    m_mainTabWidget->removeTab(idx);
    m_swimlaneTabContent->deleteLater();
    m_swimlaneTabContent = newWidget;
    int newIdx = m_mainTabWidget->addTab(m_swimlaneTabContent, tr("📊 泳道图"));
    m_mainTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    m_mainTabWidget->setCurrentIndex(newIdx);
}
