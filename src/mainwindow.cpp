#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QGraphicsDropShadowEffect>

// Modern color palette (macOS Big Sur / Ventura style)
static const char* MODERN_STYLE = R"(
/* Global Settings */
QWidget {
    font-family: -apple-system, "SF Pro Display", "SF Pro Text", "Helvetica Neue", Arial, sans-serif;
    font-size: 13px;
    color: #1d1d1f;
}

/* Main Window */
QMainWindow {
    background-color: #f5f5f7;
}

/* Menu Bar */
QMenuBar {
    background-color: rgba(255, 255, 255, 0.8);
    border-bottom: 1px solid rgba(0, 0, 0, 0.1);
    padding: 4px 8px;
    font-weight: 500;
}

QMenuBar::item {
    padding: 6px 12px;
    border-radius: 6px;
    background: transparent;
}

QMenuBar::item:selected {
    background-color: rgba(0, 0, 0, 0.08);
}

QMenuBar::item:pressed {
    background-color: rgba(0, 0, 0, 0.12);
}

QMenu {
    background-color: rgba(255, 255, 255, 0.95);
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 8px;
    padding: 6px;
}

QMenu::item {
    padding: 8px 24px;
    border-radius: 6px;
}

QMenu::item:selected {
    background-color: #007aff;
    color: white;
}

QMenu::separator {
    height: 1px;
    background-color: rgba(0, 0, 0, 0.1);
    margin: 6px 12px;
}

/* Panel Containers */
#leftPanel, #rightPanel {
    background-color: transparent;
}

/* Section Labels */
QLabel#sectionLabel {
    font-size: 15px;
    font-weight: 600;
    color: #1d1d1f;
    padding: 8px 0;
}

/* List Widget - Modern Style */
QListWidget {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 10px;
    padding: 6px;
    outline: none;
}

QListWidget::item {
    background-color: transparent;
    border-radius: 8px;
    padding: 12px 16px;
    margin: 2px 4px;
    border: none;
}

QListWidget::item:hover {
    background-color: rgba(0, 0, 0, 0.04);
}

QListWidget::item:selected {
    background-color: #007aff;
    color: white;
}

QListWidget::item:selected:hover {
    background-color: #0066d6;
}

/* Buttons - Modern Pill Style */
QPushButton {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: 500;
    min-width: 70px;
}

QPushButton:hover {
    background-color: #f5f5f7;
    border-color: rgba(0, 0, 0, 0.16);
}

QPushButton:pressed {
    background-color: #e8e8ed;
}

QPushButton:disabled {
    background-color: #f5f5f7;
    color: #86868b;
    border-color: rgba(0, 0, 0, 0.06);
}

/* Primary Button */
QPushButton#primaryBtn {
    background-color: #007aff;
    color: white;
    border: none;
}

QPushButton#primaryBtn:hover {
    background-color: #0066d6;
}

QPushButton#primaryBtn:pressed {
    background-color: #0055b3;
}

/* Text Edit / Preview Pane */
QTextEdit {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 10px;
    padding: 16px;
    font-size: 13px;
    line-height: 1.5;
    selection-background-color: rgba(0, 122, 255, 0.3);
}

QTextEdit:focus {
    border-color: rgba(0, 122, 255, 0.5);
}

/* Scroll Bars - Minimal Style */
QScrollBar:vertical {
    background-color: transparent;
    width: 10px;
    margin: 4px;
}

QScrollBar::handle:vertical {
    background-color: rgba(0, 0, 0, 0.2);
    border-radius: 5px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background-color: rgba(0, 0, 0, 0.35);
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
    background: none;
}

QScrollBar:horizontal {
    background-color: transparent;
    height: 10px;
    margin: 4px;
}

QScrollBar::handle:horizontal {
    background-color: rgba(0, 0, 0, 0.2);
    border-radius: 5px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background-color: rgba(0, 0, 0, 0.35);
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
    background: none;
}

/* Splitter */
QSplitter::handle {
    background-color: transparent;
}

QSplitter::handle:horizontal {
    width: 12px;
    margin: 0 4px;
}

QSplitter::handle:horizontal:hover {
    background-color: rgba(0, 122, 255, 0.2);
    border-radius: 2px;
}

/* Message Box */
QMessageBox {
    background-color: white;
}

QMessageBox QLabel {
    color: #1d1d1f;
}

/* Input Dialog */
QInputDialog {
    background-color: white;
}

QLineEdit {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 8px;
    padding: 8px 12px;
    selection-background-color: rgba(0, 122, 255, 0.3);
}

QLineEdit:focus {
    border-color: #007aff;
}
)";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_splitter(nullptr)
    , m_vaultList(nullptr)
    , m_previewPane(nullptr)
{
    // Apply modern style
    setStyleSheet(MODERN_STYLE);

    // Config path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_configPath = configDir + "/vaults.json";

    // Load settings
    QSettings settings;
    m_obsidianAppPath = settings.value("obsidianAppPath", "/Applications/Obsidian.app").toString();

    setupMenuBar();
    setupCentralWidget();
    loadVaults();
    updateVaultList();
}

MainWindow::~MainWindow()
{
    saveVaults();
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

    // Section header with icon
    QLabel *vaultLabel = new QLabel(tr("Vaults"));
    vaultLabel->setObjectName("sectionLabel");
    leftLayout->addWidget(vaultLabel);

    // Vault list with shadow
    m_vaultList = new QListWidget;
    m_vaultList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_vaultList->setMinimumWidth(280);
    m_vaultList->setSpacing(2);

    // Add shadow effect
    QGraphicsDropShadowEffect *listShadow = new QGraphicsDropShadowEffect;
    listShadow->setBlurRadius(20);
    listShadow->setColor(QColor(0, 0, 0, 30));
    listShadow->setOffset(0, 4);
    m_vaultList->setGraphicsEffect(listShadow);

    connect(m_vaultList, &QListWidget::itemClicked, this, &MainWindow::onVaultSelected);
    leftLayout->addWidget(m_vaultList, 1);

    // Buttons row
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);

    QPushButton *addBtn = new QPushButton(tr("Add"));
    QPushButton *removeBtn = new QPushButton(tr("Remove"));
    QPushButton *openBtn = new QPushButton(tr("Open"));
    openBtn->setObjectName("primaryBtn");

    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddVault);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveVault);
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenInObsidian);

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(openBtn);
    leftLayout->addLayout(btnLayout);

    m_splitter->addWidget(leftPanel);

    // Right: Preview panel
    QWidget *rightPanel = new QWidget;
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    rightLayout->setSpacing(12);

    // Section header
    QLabel *previewLabel = new QLabel(tr("Preview"));
    previewLabel->setObjectName("sectionLabel");
    rightLayout->addWidget(previewLabel);

    // Preview pane with shadow
    m_previewPane = new QTextEdit;
    m_previewPane->setReadOnly(true);
    m_previewPane->setPlaceholderText(tr("Select a vault to view details..."));

    // Add shadow effect
    QGraphicsDropShadowEffect *previewShadow = new QGraphicsDropShadowEffect;
    previewShadow->setBlurRadius(20);
    previewShadow->setColor(QColor(0, 0, 0, 30));
    previewShadow->setOffset(0, 4);
    m_previewPane->setGraphicsEffect(previewShadow);

    rightLayout->addWidget(m_previewPane, 1);

    m_splitter->addWidget(rightPanel);

    m_splitter->setSizes({320, 880});
    mainLayout->addWidget(m_splitter);

    setCentralWidget(centralWidget);
}

void MainWindow::loadVaults()
{
    QFile file(m_configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        m_vaults = doc.array();
        file.close();
    }
}

void MainWindow::saveVaults()
{
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_vaults);
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::updateVaultList()
{
    m_vaultList->clear();
    for (const QJsonValue &val : m_vaults) {
        QJsonObject vault = val.toObject();
        QString name = vault["name"].toString();
        QString path = vault["path"].toString();
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        m_vaultList->addItem(item);
    }
}

void MainWindow::onAddVault()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Vault Directory"));
    if (path.isEmpty()) return;

    QString name = QInputDialog::getText(this, tr("Vault Name"), tr("Enter vault name:"),
                                          QLineEdit::Normal, QDir(path).dirName());
    if (name.isEmpty()) return;

    QJsonObject vault;
    vault["name"] = name;
    vault["path"] = path;
    vault["addedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_vaults.append(vault);
    saveVaults();
    updateVaultList();
}

void MainWindow::onRemoveVault()
{
    QListWidgetItem *item = m_vaultList->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("Warning"), tr("No vault selected."));
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    for (int i = 0; i < m_vaults.size(); ++i) {
        if (m_vaults[i].toObject()["path"].toString() == path) {
            m_vaults.removeAt(i);
            break;
        }
    }
    saveVaults();
    updateVaultList();
    m_previewPane->clear();
}

void MainWindow::onOpenVault()
{
    onOpenInObsidian();
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

void MainWindow::openVaultInObsidian(const QString &vaultPath)
{
    QString appPath = getObsidianAppPath();
    if (appPath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Obsidian app path not configured.\nPlease set it in Settings."));
        return;
    }

    // macOS: open Obsidian with vault path
    QProcess process;
    QStringList args;
    args << "-a" << appPath << vaultPath;
    process.startDetached("open", args);
}

QString MainWindow::getObsidianAppPath()
{
    return m_obsidianAppPath;
}

void MainWindow::onSettings()
{
    QString path = QInputDialog::getText(this, tr("Obsidian App Path"),
                                         tr("Enter Obsidian.app path:"),
                                         QLineEdit::Normal, m_obsidianAppPath);
    if (!path.isEmpty()) {
        m_obsidianAppPath = path;
        QSettings settings;
        settings.setValue("obsidianAppPath", path);
    }
}

void MainWindow::onVaultSelected(QListWidgetItem *item)
{
    QString path = item->data(Qt::UserRole).toString();
    QString name = item->text();

    // Show vault info with formatted output
    QString info;
    info += tr("<h2 style='color: #1d1d1f; margin-bottom: 16px;'>%1</h2>").arg(name);
    info += tr("<p style='color: #86868b; font-size: 12px; margin-bottom: 16px;'>%1</p>").arg(path);

    QDir vaultDir(path);
    if (vaultDir.exists()) {
        int files = vaultDir.entryList(QDir::Files | QDir::NoDotAndDotDot).size();
        int folders = vaultDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();

        info += tr("<div style='background: #f5f5f7; border-radius: 8px; padding: 12px; margin-bottom: 12px;'>");
        info += tr("<p style='margin: 4px 0;'><b>Files:</b> %1</p>").arg(files);
        info += tr("<p style='margin: 4px 0;'><b>Folders:</b> %1</p>").arg(folders);
        info += tr("</div>");

        // Check for .obsidian folder
        if (vaultDir.exists(".obsidian")) {
            info += tr("<p style='color: #34c759; font-weight: 500;'>✓ Obsidian configuration detected</p>");
        }
    } else {
        info += tr("<p style='color: #ff3b30;'>⚠ Path does not exist</p>");
    }

    m_previewPane->setHtml(info);
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
                          "<li>Quick vault switching</li>"
                          "<li>Open vaults in Obsidian</li>"
                          "<li>Preview vault details</li>"
                          "</ul>"
                          "<p style='color: #86868b; margin-top: 16px;'>Built with Qt 6.10.0</p>"));
}