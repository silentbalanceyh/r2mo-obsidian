#include "mainwindow.h"
#include "theme/thememanager.h"
#include "models/vaultmodel.h"
#include "utils/settingsmanager.h"
#include "utils/vaultvalidator.h"
#include "i18n/translationmanager.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
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
    , m_openBtn(nullptr)
    , m_previewLabel(nullptr)
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
    m_vaultList->setSpacing(2);

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
    m_removeBtn = new QPushButton(tr("Remove"));
    m_openBtn = new QPushButton(tr("Open"));
    m_openBtn->setObjectName("primaryBtn");
    
    // Fix button height to prevent flickering on language change
    m_addBtn->setFixedHeight(32);
    m_removeBtn->setFixedHeight(32);
    m_openBtn->setFixedHeight(32);

    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddVault);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveVault);
    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::onOpenInObsidian);

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_openBtn);
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
    m_previewLabel->setFixedHeight(36);  // Fixed height: font 15px + padding 16px + margin
    rightLayout->addWidget(m_previewLabel);

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
        m_vaultList->addItem(item);
    }
}

void MainWindow::onAddVault()
{
    // Use non-native dialog to show hidden files (.obsidian, .r2mo, etc.)
    QFileDialog dialog(this, tr("Select Obsidian Vault Directory"));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    
    // Show hidden files/directories so user can see .obsidian folder
    dialog.setFilter(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
    dialog.setDirectory(QDir::homePath());
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QStringList selected = dialog.selectedFiles();
    if (selected.isEmpty()) {
        return;
    }
    
    QString path = selected.first();
    
    // Validate: must contain .obsidian directory
    if (!m_vaultValidator->isValidVault(path)) {
        QMessageBox::warning(this, tr("Invalid Vault"),
            tr("<h3>Not a Valid Obsidian Vault</h3>"
               "<p>The selected directory does not contain a <b>.obsidian</b> folder.</p>"
               "<p style='color: #86868b;'>Only directories that have been opened in Obsidian "
               "(containing the .obsidian configuration folder) can be added as vaults.</p>"
               "<p><b>Selected path:</b><br><code style='background: #f5f5f7; padding: 4px 8px; border-radius: 4px;'>%1</code></p>")
            .arg(path));
        return;
    }

    QString name = QInputDialog::getText(this, tr("Vault Name"), tr("Enter vault name:"),
                                          QLineEdit::Normal, QDir(path).dirName());
    if (name.isEmpty()) return;

    Vault vault;
    vault.name = name;
    vault.path = path;
    vault.addedAt = QDateTime::currentDateTime();

    m_vaultModel->addVault(vault);
    m_vaultModel->save(m_configPath);
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
    QString info;
    info += tr("<h2 style='color: #1d1d1f; margin-bottom: 16px;'>%1</h2>").arg(name);
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
    m_openBtn->setText(tr("Open"));
    
    // Retranslate placeholder
    m_previewPane->setPlaceholderText(tr("Select a vault to view details..."));
    
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