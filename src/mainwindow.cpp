#include "mainwindow.h"
#include "theme/thememanager.h"
#include "models/vaultmodel.h"
#include "utils/settingsmanager.h"
#include "utils/vaultvalidator.h"
#include "utils/obsidianconfigreader.h"
#include "utils/r2moscanner.h"
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
#include <QGraphicsDropShadowEffect>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QButtonGroup>
#include <QTabWidget>
#include <QWheelEvent>

MainWindow::MainWindow(QWidget *parent)
     : QMainWindow(parent)
     , m_toolBar(nullptr)
     , m_themeBtn(nullptr)
     , m_langGroup(nullptr)
     , m_btnZh(nullptr)
     , m_btnEn(nullptr)
    , m_splitter(nullptr)
    , m_vaultLabel(nullptr)
    , m_vaultList(nullptr)
    , m_addBtn(nullptr)
    , m_removeBtn(nullptr)
    , m_previewLabel(nullptr)
    , m_previewHeader(nullptr)
    , m_previewTitle(nullptr)
    , m_previewEditBtn(nullptr)
    , m_previewOpenBtn(nullptr)
    , m_previewTitleEdit(nullptr)
    , m_previewPane(nullptr)
    , m_tabWidget(nullptr)
    , m_overviewTab(nullptr)
    , m_tasksTab(nullptr)
    , m_graphTab(nullptr)
    , m_graphView(nullptr)
    , m_graphScene(nullptr)
    , m_taskTree(nullptr)
    , m_vaultModel(nullptr)
    , m_settingsManager(nullptr)
    , m_vaultValidator(nullptr)
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
    mainLayout->setSpacing(0);

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
    m_vaultLabel->setFixedHeight(36);  // Fixed height: font 15px + padding 16px + margin
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

    // Buttons row
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);

    m_addBtn = new QPushButton(tr("Add"));
    m_addBtn->setObjectName("primaryBtn");
    m_removeBtn = new QPushButton(tr("Remove"));
    
    // Fix button height to prevent flickering on language change
    m_addBtn->setFixedHeight(32);
    m_removeBtn->setFixedHeight(32);

    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddVault);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveVault);

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addStretch();
    leftLayout->addLayout(btnLayout);

    m_splitter->addWidget(leftPanel);

    // Right: Tab widget with three tabs
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

    // Create tab widget
    m_tabWidget = new QTabWidget;
    m_tabWidget->setObjectName("previewTabs");
    // Tab styling: left-aligned, trapezoid shape (no rounded corners), active tab matches page background
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: none; background: white; }"
        "QTabWidget::tab-bar { alignment: left; }"
        "QTabBar::tab { padding: 8px 20px; margin-right: 0px; border: 1px solid #e0e0e0; border-bottom: none; border-radius: 0px; background: #f5f5f7; }"
        "QTabBar::tab:selected { background: white; color: #007aff; border-bottom: 1px solid white; }"
        "QTabBar::tab:!selected { background: #e8e8ed; color: #86868b; margin-top: 2px; }"
    );
    
    // Tab 1: Overview (概览)
    m_overviewTab = new QWidget;
    QVBoxLayout *overviewLayout = new QVBoxLayout(m_overviewTab);
    overviewLayout->setContentsMargins(0, 0, 0, 0);
    overviewLayout->setSpacing(0);
    
    m_previewPane = new QTextEdit;
    m_previewPane->setReadOnly(true);
    m_previewPane->setPlaceholderText(tr("Select a vault to view details..."));
    m_previewPane->setStyleSheet("QTextEdit { border: none; background: white; }");

    overviewLayout->addWidget(m_previewPane, 1);
    m_tabWidget->addTab(m_overviewTab, tr("概览"));
    
    // Tab 2: Project Tasks (项目任务)
    m_tasksTab = new QWidget;
    QVBoxLayout *tasksLayout = new QVBoxLayout(m_tasksTab);
    tasksLayout->setContentsMargins(0, 0, 0, 0);
    tasksLayout->setSpacing(0);
    
    m_taskTree = new QTreeWidget;
    m_taskTree->setHeaderLabel(tr("Tasks"));
    m_taskTree->setStyleSheet("QTreeWidget { border: none; background: white; } QTreeWidget::item { padding: 4px; } QTreeWidget::item:selected { background: #007aff; color: white; }");
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

    rightLayout->addWidget(m_tabWidget, 1);

    m_splitter->addWidget(rightPanel);

    m_splitter->setSizes({400, 1280});
    mainLayout->addWidget(m_splitter);

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
    connect(m_vaultModel, &VaultModel::modelChanged, this, &MainWindow::updateVaultList);
    connect(TranslationManager::instance(), &TranslationManager::languageChanged, 
            this, &MainWindow::onLanguageChanged);
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
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
    m_previewPane->clear();
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
        m_previewPane->clear();
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
    QString appPath = m_settingsManager->obsidianAppPath();
    if (appPath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), 
            tr("Obsidian app path not configured.\nPlease set it in Settings."));
        return;
    }

    // macOS: open Obsidian with vault path
    QProcess process;
    QStringList args;
    args << "-a" << appPath << vaultPath;
    process.startDetached("open", args);
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
    
    QString info;
    
    // Table style - full width
    info += tr("<style>"
        "body { margin: 0; padding: 0; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 0; table-layout: fixed; }"
        "th { text-align: left; padding: 14px 20px; background: #f5f5f7; font-weight: 600; border-bottom: 2px solid #e0e0e0; }"
        "td { padding: 14px 20px; border-bottom: 1px solid #e8e8ed; }"
        "tr:hover { background: #fafafa; }"
        ".label { color: #86868b; width: 50%; }"
        ".value { font-weight: 500; width: 50%; }"
        ".status-ok { color: #34c759; }"
        ".status-info { color: #007aff; }"
        ".status-warning { color: #ff9500; }"
        ".path-bar { color: #86868b; font-size: 12px; padding: 14px 20px; background: #f5f5f7; border-bottom: 1px solid #e0e0e0; }"
        "</style>");
    
    // Path info
    info += tr("<div class='path-bar'>%1</div>").arg(path);

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

        // Vault Statistics Table
        info += tr("<table>");
        info += tr("<tr><th colspan='2'>📊 %1</th></tr>").arg(tr("Vault Statistics"));
        info += tr("<tr><td class='label'>%1</td><td class='value'>%2</td></tr>").arg(tr("Files")).arg(files);
        info += tr("<tr><td class='label'>%1</td><td class='value'>%2</td></tr>").arg(tr("Folders")).arg(folders);
        if (hiddenItems > 0) {
            info += tr("<tr><td class='label'>%1</td><td class='value'>%2 <span style='color: #86868b;'>(.obsidian, .r2mo)</span></td></tr>").arg(tr("Hidden Items")).arg(hiddenItems);
        }
        
        // Check for .obsidian folder
        if (m_vaultValidator->hasObsidianConfig(path)) {
            info += tr("<tr><td class='label'>%1</td><td class='value status-ok'>✓ %2</td></tr>").arg(tr("Obsidian")).arg(tr("Valid Vault"));
        }
        
        info += tr("</table>");
        
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
                
                // R2MO Statistics Table
                info += tr("<table>");
                info += tr("<tr><th colspan='2'>🔧 %1</th></tr>").arg(tr("R2MO Statistics"));
                info += tr("<tr><td class='label'>%1</td><td class='value status-info'>✓ %2</td></tr>").arg(tr("Configuration")).arg(tr("Detected"));
                info += tr("<tr><td class='label'>%1</td><td class='value'>%2</td></tr>").arg(tr("Total Projects")).arg(projects.size());
                info += tr("<tr><td class='label'>%1</td><td class='value status-warning'>%2 %3</td></tr>").arg(tr("Task Queue")).arg(totalQueue).arg(tr("pending"));
                info += tr("<tr><td class='label'>%1</td><td class='value status-ok'>%2 %3</td></tr>").arg(tr("Historical Tasks")).arg(totalHistorical).arg(tr("completed"));
                info += tr("</table>");
                
                // Draw directed graph for project relationships (in Graph tab)
                drawProjectGraph(projects);
                
                // Build expandable task tree (in Tasks tab)
                buildTaskTree(projects);
            }
        }
    } else {
        info += tr("<table>");
        info += tr("<tr><th colspan='2'>⚠ %1</th></tr>").arg(tr("Warning"));
        info += tr("<tr><td colspan='2' style='color: #ff3b30;'>%1</td></tr>").arg(tr("Path does not exist"));
        info += tr("</table>");
    }

    m_previewPane->setHtml(info);
}

void MainWindow::drawProjectGraph(const QList<R2moSubProject>& projects)
{
    m_graphScene->clear();
    
    // Determine colors based on current theme
    ThemeManager::Theme currentTheme = ThemeManager::instance()->currentTheme();
    
    // Text colors: dark text for light backgrounds, light text for dark backgrounds
    QColor textColor = (currentTheme == ThemeManager::Light) ? QColor("#333333") : QColor("#f5f5f7");
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
    
    // Set font for text items
    QFont textFont("MesloLGS NF", 12);
    textFont.setWeight(QFont::DemiBold);
    
    // Calculate layout based on view size
    QSize viewSize = m_graphView->viewport()->size();
    int viewWidth = viewSize.width();
    int viewHeight = viewSize.height();
    
    // Node dimensions
    int nodeWidth = 160;
    int nodeHeight = 60;
    int margin = 80;
    
    // Calculate positions
    int parentY = margin;
    int childY = margin + 150;
    
    // Center parent horizontally
    int parentX = viewWidth / 2;
    
    // Draw parent node
    QGraphicsRectItem* parentNode = m_graphScene->addRect(
        parentX - nodeWidth/2, parentY, nodeWidth, nodeHeight,
        QPen(borderColor, 2), QBrush(parentBgColor));
    
    QGraphicsTextItem* parentText = m_graphScene->addText(parent->name, textFont);
    parentText->setDefaultTextColor(textColor);
    parentText->setPos(parentX - parentText->boundingRect().width()/2, parentY + 18);
    
    // Draw children nodes and arrows
    int childSpacing = qMax(180, viewWidth / (children.size() + 1));
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
        childText->setPos(childX - childText->boundingRect().width()/2, childY + 18);
    }
    
    // Add legend with proper text color
    QFont legendFont("MesloLGS NF", 10);
    QGraphicsTextItem* legend = m_graphScene->addText(
        tr("Ctrl+Scroll: Zoom | Drag: Pan | Double-click: Reset"), legendFont);
    legend->setDefaultTextColor(QColor("#86868b"));
    legend->setPos(10, 10);
    
    // Set scene rect to fill the entire view
    m_graphScene->setSceneRect(0, 0, viewWidth, viewHeight);
    
    // Fit the view to show all content
    m_graphView->fitInView(m_graphScene->sceneRect(), Qt::KeepAspectRatio);
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
    
    // Create parent project item as top-level
    QTreeWidgetItem* parentItem = new QTreeWidgetItem(m_taskTree);
    parentItem->setText(0, QString("📁 %1").arg(parent->name));
    parentItem->setData(0, Qt::UserRole, parent->path);
    QFont parentFont = parentItem->font(0);
    parentFont.setBold(true);
    parentItem->setFont(0, parentFont);
    
    QString parentR2moPath = parent->path + "/.r2mo";
    
    // Add sub-projects under parent
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
    
    return QMainWindow::eventFilter(watched, event);
}