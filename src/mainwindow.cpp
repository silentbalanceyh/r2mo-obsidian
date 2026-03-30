#include "mainwindow.h"
#include "theme/thememanager.h"
#include "models/vaultmodel.h"
#include "utils/settingsmanager.h"
#include "utils/vaultvalidator.h"
#include "utils/obsidianconfigreader.h"
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
#include <QGraphicsDropShadowEffect>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QButtonGroup>

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
    , m_previewTitleEdit(nullptr)
    , m_previewPane(nullptr)
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

    // Right: Preview panel
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

    // Preview header with title and edit button
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
    
    m_previewTitleEdit = new QLineEdit;
    m_previewTitleEdit->setObjectName("previewTitleEdit");
    m_previewTitleEdit->setStyleSheet("QLineEdit#previewTitleEdit { font-size: 18px; font-weight: 600; background: white; border: 2px solid #007aff; border-radius: 3px; padding: 4px 8px; }");
    m_previewTitleEdit->hide();
    headerLayout->addWidget(m_previewTitleEdit, 1);
    
    rightLayout->addWidget(m_previewHeader);

    // Preview pane with shadow
    m_previewPane = new QTextEdit;
    m_previewPane->setReadOnly(true);
    m_previewPane->setPlaceholderText(tr("Select a vault to view details..."));

    QGraphicsDropShadowEffect *previewShadow = new QGraphicsDropShadowEffect;
    previewShadow->setBlurRadius(20);
    previewShadow->setColor(QColor(0, 0, 0, 30));
    previewShadow->setOffset(0, 4);
    m_previewPane->setGraphicsEffect(previewShadow);

    rightLayout->addWidget(m_previewPane, 1);

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
    connect(m_previewTitleEdit, &QLineEdit::editingFinished, this, &MainWindow::onPreviewTitleEditFinished);
    connect(m_previewTitleEdit, &QLineEdit::returnPressed, this, &MainWindow::onPreviewTitleReturnPressed);
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
    
    // Update title and show edit button
    m_previewTitle->setText(name);
    m_previewTitle->show();
    m_previewEditBtn->show();
    m_previewTitleEdit->hide();
    
    QString info;
    info += tr("<p style='color: #86868b; font-size: 12px; margin-bottom: 16px;'>%1</p>").arg(path);

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

        info += tr("<div style='background: #f5f5f7; border-radius: 8px; padding: 12px; margin-bottom: 12px;'>");
        info += tr("<p style='margin: 4px 0;'><b>Files:</b> %1</p>").arg(files);
        info += tr("<p style='margin: 4px 0;'><b>Folders:</b> %1</p>").arg(folders);
        if (hiddenItems > 0) {
            info += tr("<p style='margin: 4px 0; color: #86868b;'><b>Hidden items:</b> %1 (e.g. .obsidian, .r2mo)</p>").arg(hiddenItems);
        }
        info += tr("</div>");

        // Check for .obsidian folder
        if (m_vaultValidator->hasObsidianConfig(path)) {
            info += tr("<p style='color: #34c759; font-weight: 500;'>✓ Valid Obsidian Vault</p>");
        }
        
        // Check for .r2mo folder
        if (m_vaultValidator->hasR2moConfig(path)) {
            info += tr("<p style='color: #007aff; font-weight: 500;'>✓ R2MO configuration detected</p>");
        }
    } else {
        info += tr("<p style='color: #ff3b30;'>⚠ Path does not exist</p>");
    }

    m_previewPane->setHtml(info);
}

void MainWindow::onPreviewEditClicked()
{
    if (m_currentPreviewPath.isEmpty()) return;
    
    // Switch to edit mode
    m_previewTitleEdit->setText(m_previewTitle->text());
    m_previewTitle->hide();
    m_previewEditBtn->hide();
    m_previewTitleEdit->show();
    m_previewTitleEdit->setFocus();
    m_previewTitleEdit->selectAll();
}

void MainWindow::onPreviewTitleEditFinished()
{
    QString newName = m_previewTitleEdit->text().trimmed();
    
    // Switch back to display mode
    m_previewTitle->show();
    m_previewEditBtn->show();
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