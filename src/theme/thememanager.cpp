#include "thememanager.h"
#include <QSettings>

namespace {
QString encodedColor(QString color)
{
    return color.replace("#", "%23");
}

QString branchChevronDataUri(const QString& color, bool open)
{
    const QString path = open
        ? QStringLiteral("M4.5 6L8 10L11.5 6")
        : QStringLiteral("M6 4.5L10 8L6 11.5");

    return QStringLiteral(
        "url(\"data:image/svg+xml;utf8,"
        "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 16 16'>"
        "<path d='%1' fill='none' stroke='%2' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'/>"
        "</svg>\")")
        .arg(path, encodedColor(color));
}

QString branchIndicatorStyle(const QString& branchColor, const QString& selectedBranchColor)
{
    return QStringLiteral(R"(
/* Branch indicators - themed and shared by all tree controls */
QTreeWidget::branch,
QTreeView::branch {
    background-color: transparent;
}

QTreeWidget::branch:has-children:!has-siblings:closed,
QTreeWidget::branch:closed:has-children:has-siblings,
QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    border-image: none;
    image: %1;
}

QTreeWidget::branch:open:has-children:!has-siblings,
QTreeWidget::branch:open:has-children:has-siblings,
QTreeView::branch:open:has-children:!has-siblings,
QTreeView::branch:open:has-children:has-siblings {
    border-image: none;
    image: %2;
}

QTreeWidget::branch:selected:has-children:!has-siblings:closed,
QTreeWidget::branch:selected:closed:has-children:has-siblings,
QTreeView::branch:selected:has-children:!has-siblings:closed,
QTreeView::branch:selected:closed:has-children:has-siblings {
    border-image: none;
    image: %3;
}

QTreeWidget::branch:selected:open:has-children:!has-siblings,
QTreeWidget::branch:selected:open:has-children:has-siblings,
QTreeView::branch:selected:open:has-children:!has-siblings,
QTreeView::branch:selected:open:has-children:has-siblings {
    border-image: none;
    image: %4;
}

QTreeWidget::branch:has-siblings:!has-children,
QTreeWidget::branch:has-siblings:has-children,
QTreeView::branch:has-siblings:!has-children,
QTreeView::branch:has-siblings:has-children,
QTreeWidget::branch:has-siblings:adjoins-item,
QTreeWidget::branch:!has-children:!has-siblings:adjoins-item,
QTreeView::branch:has-siblings:adjoins-item,
QTreeView::branch:!has-children:!has-siblings:adjoins-item {
    border-image: none;
}
)")
        .arg(branchChevronDataUri(branchColor, false),
             branchChevronDataUri(branchColor, true),
             branchChevronDataUri(selectedBranchColor, false),
             branchChevronDataUri(selectedBranchColor, true));
}
}

ThemeManager* ThemeManager::instance()
{
    static ThemeManager manager;
    return &manager;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , m_currentTheme(Light)
{
    m_themeNames[Light] = "Light";
    m_themeNames[Dark] = "Dark";
}

void ThemeManager::initialize()
{
    QSettings settings;
    int savedTheme = settings.value("theme", static_cast<int>(Light)).toInt();
    setTheme(static_cast<Theme>(savedTheme));
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        QSettings settings;
        settings.setValue("theme", static_cast<int>(theme));
        emit themeChanged(theme);
    }
}

ThemeManager::Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

QString ThemeManager::currentThemeName() const
{
    return m_themeNames.value(m_currentTheme, "Light");
}

QFont ThemeManager::uiFont()
{
    // PingFang SC for Chinese UI text (14px)
    return QFont("PingFang SC", 14);
}

QFont ThemeManager::monoFont()
{
    // MesloLGS NF for English text (15px)
    QFont font("MesloLGS NF", 15);
    font.setStyleHint(QFont::Monospace);
    return font;
}

QString ThemeManager::lightStyle() const
{
    return QStringLiteral(R"(
/* Light Theme - Global Settings */
QWidget {
    font-family: "MesloLGS NF", "PingFang SC", sans-serif;
    font-size: 14px;
    color: #1d1d1f;
}

QMainWindow {
    background-color: #f5f5f7;
}

/* Panel Containers */
#leftPanel, #rightPanel {
    background-color: transparent;
}

/* Section Labels */
QLabel#sectionLabel {
    font-size: 16px;
    font-weight: 600;
    color: #1d1d1f;
    padding: 8px 0;
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
    border-radius: 3px;
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
    border-radius: 3px;
    padding: 6px;
}

QMenu::item {
    padding: 8px 24px;
    border-radius: 3px;
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

/* List Widget */
QListWidget {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 3px;
    padding: 6px;
    outline: none;
}

QListWidget::item {
    background-color: transparent;
    border-radius: 3px;
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

/* Buttons */
QPushButton {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
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

/* Text Edit */
QTextEdit {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 3px;
    padding: 16px;
    font-size: 15px;
    line-height: 1.5;
    selection-background-color: rgba(0, 122, 255, 0.3);
}

QTextEdit:focus {
    border-color: rgba(0, 122, 255, 0.5);
}

/* Scroll Bars */
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

/* Generic Dialog */
QDialog {
    background-color: white;
}

/* Tree Widget */
QTreeWidget {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 3px;
    outline: none;
}

QTreeWidget::item {
    padding: 6px 8px;
    border-radius: 3px;
}

QTreeWidget::item:selected {
    background-color: #007aff;
    color: white;
}

QTreeWidget::item:hover {
    background-color: rgba(0, 0, 0, 0.04);
}

QTreeWidget::item:selected:hover {
    background-color: #0066d6;
}

%1

QHeaderView::section {
    background-color: #f5f5f7;
    border: none;
    border-bottom: 1px solid rgba(0, 0, 0, 0.08);
    padding: 8px 12px;
    font-weight: 600;
}

QHeaderView::section:hover {
    background-color: #ebebed;
}

/* File Dialog */
QFileDialog {
    background-color: white;
}

QFileDialog QLabel {
    color: #1d1d1f;
}

QFileDialog QPushButton {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    padding: 6px 16px;
    font-weight: 500;
    min-width: 70px;
}

QFileDialog QPushButton:hover {
    background-color: #f5f5f7;
}

QFileDialog QPushButton:default {
    background-color: #007aff;
    color: white;
    border: none;
}

QFileDialog QListView {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 3px;
}

QFileDialog QTreeView {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 3px;
}

QLineEdit {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    padding: 8px 12px;
    selection-background-color: rgba(0, 122, 255, 0.3);
}

QLineEdit:focus {
    border-color: #007aff;
}

QComboBox {
    background-color: white;
    color: #1d1d1f;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    padding: 8px 12px;
    min-height: 20px;
}

QComboBox:hover {
    border-color: rgba(0, 0, 0, 0.18);
}

QComboBox:focus {
    border-color: #007aff;
}

QComboBox::drop-down {
    border: none;
    width: 28px;
    background: transparent;
}

QComboBox QAbstractItemView {
    background-color: white;
    color: #1d1d1f;
    border: 1px solid rgba(0, 0, 0, 0.12);
    selection-background-color: #007aff;
    selection-color: white;
    outline: none;
}

/* ToolBar */
QToolBar {
    background-color: rgba(255, 255, 255, 0.9);
    border-bottom: 1px solid rgba(0, 0, 0, 0.1);
    padding: 4px 8px;
    spacing: 4px;
}

QToolBar QToolButton {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    padding: 6px 16px;
    font-weight: 500;
    min-width: 50px;
}

QToolBar QToolButton:hover {
    background-color: rgba(0, 0, 0, 0.04);
}

QToolBar QToolButton:checked {
    background-color: #007aff;
    color: white;
    border-color: #007aff;
}

/* Theme Toggle Button (Square) */
QPushButton#themeBtn {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    font-size: 16px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#themeBtn:hover {
    background-color: rgba(0, 0, 0, 0.04);
}

QPushButton#themeBtn:pressed {
    background-color: rgba(0, 0, 0, 0.08);
}

/* Language Button Group - no border, integrated look */
#langGroupWidget {
    background-color: transparent;
    border: none;
}

/* Language buttons - segmented control style */
QPushButton#langBtnLeft {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-right: none;
    border-top-left-radius: 3px;
    border-bottom-left-radius: 3px;
    border-top-right-radius: 0px;
    border-bottom-right-radius: 0px;
    font-size: 14px;
    font-weight: 500;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#langBtnLeft:hover {
    background-color: rgba(0, 0, 0, 0.06);
}

QPushButton#langBtnLeft:checked {
    background-color: #007aff;
    color: white;
}

QPushButton#langBtnRight {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-top-left-radius: 0px;
    border-bottom-left-radius: 0px;
    border-top-right-radius: 3px;
    border-bottom-right-radius: 3px;
    font-size: 14px;
    font-weight: 500;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#langBtnRight:hover {
    background-color: rgba(0, 0, 0, 0.06);
}

QPushButton#langBtnRight:checked {
    background-color: #007aff;
    color: white;
}

/* Edit Button (small icon button) */
QPushButton#editBtn {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    font-size: 14px;
    min-width: 28px;
    max-width: 28px;
    min-height: 28px;
    max-height: 28px;
    padding: 0px;
}

QPushButton#editBtn:hover {
    background-color: rgba(0, 0, 0, 0.04);
}

QPushButton#editBtn:pressed {
    background-color: rgba(0, 0, 0, 0.08);
}

/* Toolbar icon buttons - Add/Remove segmented control */
QPushButton#addBtnLeft {
    background-color: #007aff;
    color: white;
    border: 1px solid #007aff;
    border-right: none;
    border-top-left-radius: 3px;
    border-bottom-left-radius: 3px;
    border-top-right-radius: 0px;
    border-bottom-right-radius: 0px;
    font-size: 16px;
    font-weight: bold;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#addBtnLeft:hover {
    background-color: #0056cc;
}

QPushButton#addRemoteBtnMiddle {
    background-color: #34c759;
    color: white;
    border: 1px solid #34c759;
    border-right: none;
    border-radius: 0px;
    font-size: 15px;
    font-weight: bold;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#addRemoteBtnMiddle:hover {
    background-color: #28a745;
}

QPushButton#removeBtnRight {
    background-color: transparent;
    color: #ff3b30;
    border: 1px solid #ff3b30;
    border-top-left-radius: 0px;
    border-bottom-left-radius: 0px;
    border-top-right-radius: 3px;
    border-bottom-right-radius: 3px;
    font-size: 16px;
    font-weight: bold;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#removeBtnRight:hover {
    background-color: #ff3b30;
    color: white;
}

QPushButton#swimlaneBtn {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    font-size: 16px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#swimlaneBtn:hover {
    background-color: rgba(0, 0, 0, 0.04);
}

QPushButton#monitorBtn {
    background-color: transparent;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 3px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#monitorBtn:hover {
    background-color: rgba(0, 0, 0, 0.04);
}
)")
        .arg(branchIndicatorStyle("#3a3a3c", "#ffffff"));
}

QString ThemeManager::darkStyle() const
{
    return QStringLiteral(R"(
/* Dark Theme - Global Settings */
QWidget {
    font-family: "MesloLGS NF", "PingFang SC", sans-serif;
    font-size: 14px;
    color: #f5f5f7;
}

QMainWindow {
    background-color: #1c1c1e;
}

/* Panel Containers */
#leftPanel, #rightPanel {
    background-color: transparent;
}

/* Section Labels */
QLabel#sectionLabel {
    font-size: 16px;
    font-weight: 600;
    color: #f5f5f7;
    padding: 8px 0;
}

/* Menu Bar */
QMenuBar {
    background-color: rgba(28, 28, 30, 0.9);
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    padding: 4px 8px;
    font-weight: 500;
}

QMenuBar::item {
    padding: 6px 12px;
    border-radius: 3px;
    background: transparent;
}

QMenuBar::item:selected {
    background-color: rgba(255, 255, 255, 0.1);
}

QMenuBar::item:pressed {
    background-color: rgba(255, 255, 255, 0.15);
}

QMenu {
    background-color: rgba(44, 44, 46, 0.95);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 6px;
}

QMenu::item {
    padding: 8px 24px;
    border-radius: 3px;
}

QMenu::item:selected {
    background-color: #0a84ff;
    color: white;
}

QMenu::separator {
    height: 1px;
    background-color: rgba(255, 255, 255, 0.1);
    margin: 6px 12px;
}

/* List Widget */
QListWidget {
    background-color: #2c2c2e;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 6px;
    outline: none;
}

QListWidget::item {
    background-color: transparent;
    border-radius: 3px;
    padding: 12px 16px;
    margin: 2px 4px;
    border: none;
}

QListWidget::item:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

QListWidget::item:selected {
    background-color: #0a84ff;
    color: white;
}

QListWidget::item:selected:hover {
    background-color: #0070e0;
}

/* Buttons */
QPushButton {
    background-color: #3a3a3c;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 8px 16px;
    font-weight: 500;
    min-width: 70px;
    color: #f5f5f7;
}

QPushButton:hover {
    background-color: #48484a;
    border-color: rgba(255, 255, 255, 0.15);
}

QPushButton:pressed {
    background-color: #545456;
}

QPushButton:disabled {
    background-color: #2c2c2e;
    color: #636366;
    border-color: rgba(255, 255, 255, 0.05);
}

QPushButton#primaryBtn {
    background-color: #0a84ff;
    color: white;
    border: none;
}

QPushButton#primaryBtn:hover {
    background-color: #0070e0;
}

QPushButton#primaryBtn:pressed {
    background-color: #005bb5;
}

/* Text Edit */
QTextEdit {
    background-color: #2c2c2e;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 16px;
    font-size: 15px;
    line-height: 1.5;
    color: #f5f5f7;
    selection-background-color: rgba(10, 132, 255, 0.3);
}

QTextEdit:focus {
    border-color: rgba(10, 132, 255, 0.5);
}

/* Scroll Bars */
QScrollBar:vertical {
    background-color: transparent;
    width: 10px;
    margin: 4px;
}

QScrollBar::handle:vertical {
    background-color: rgba(255, 255, 255, 0.2);
    border-radius: 5px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background-color: rgba(255, 255, 255, 0.35);
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
    background-color: rgba(255, 255, 255, 0.2);
    border-radius: 5px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background-color: rgba(255, 255, 255, 0.35);
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
    background-color: rgba(10, 132, 255, 0.2);
    border-radius: 2px;
}

/* Message Box */
QMessageBox {
    background-color: #2c2c2e;
}

QMessageBox QLabel {
    color: #f5f5f7;
}

/* Input Dialog */
QInputDialog {
    background-color: #2c2c2e;
}

/* Generic Dialog */
QDialog {
    background-color: #2c2c2e;
}

/* Tree Widget */
QTreeWidget {
    background-color: #2c2c2e;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    outline: none;
    color: #f5f5f7;
}

QTreeWidget::item {
    padding: 6px 8px;
    border-radius: 3px;
}

QTreeWidget::item:selected {
    background-color: #0a84ff;
    color: white;
}

QTreeWidget::item:hover {
    background-color: rgba(255, 255, 255, 0.06);
}

QTreeWidget::item:selected:hover {
    background-color: #0066cc;
}

%1

QHeaderView::section {
    background-color: #3a3a3c;
    border: none;
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    padding: 8px 12px;
    font-weight: 600;
    color: #f5f5f7;
}

QHeaderView::section:hover {
    background-color: #4a4a4c;
}

/* File Dialog */
QFileDialog {
    background-color: #2c2c2e;
}

QFileDialog QLabel {
    color: #f5f5f7;
}

QFileDialog QPushButton {
    background-color: #3a3a3c;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 6px 16px;
    font-weight: 500;
    min-width: 70px;
    color: #f5f5f7;
}

QFileDialog QPushButton:hover {
    background-color: #4a4a4c;
}

QFileDialog QPushButton:default {
    background-color: #0a84ff;
    color: white;
    border: none;
}

QFileDialog QListView {
    background-color: #2c2c2e;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    color: #f5f5f7;
}

QFileDialog QTreeView {
    background-color: #2c2c2e;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    color: #f5f5f7;
}

QLineEdit {
    background-color: #3a3a3c;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 8px 12px;
    color: #f5f5f7;
    selection-background-color: rgba(10, 132, 255, 0.3);
}

QLineEdit:focus {
    border-color: #0a84ff;
}

QComboBox {
    background-color: #3a3a3c;
    color: #f5f5f7;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 8px 12px;
    min-height: 20px;
}

QComboBox:hover {
    border-color: rgba(255, 255, 255, 0.18);
}

QComboBox:focus {
    border-color: #0a84ff;
}

QComboBox::drop-down {
    border: none;
    width: 28px;
    background: transparent;
}

QComboBox QAbstractItemView {
    background-color: #2c2c2e;
    color: #f5f5f7;
    border: 1px solid rgba(255, 255, 255, 0.1);
    selection-background-color: #0a84ff;
    selection-color: white;
    outline: none;
}

/* ToolBar */
QToolBar {
    background-color: rgba(44, 44, 46, 0.9);
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    padding: 4px 8px;
    spacing: 4px;
}

QToolBar QToolButton {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    padding: 6px 16px;
    font-weight: 500;
    min-width: 50px;
    color: #f5f5f7;
}

QToolBar QToolButton:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

QToolBar QToolButton:checked {
    background-color: #0a84ff;
    color: white;
    border-color: #0a84ff;
}

/* Theme Toggle Button (Square) */
QPushButton#themeBtn {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    font-size: 16px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#themeBtn:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

QPushButton#themeBtn:pressed {
    background-color: rgba(255, 255, 255, 0.08);
}

/* Language Button Group - no border, integrated look */
#langGroupWidget {
    background-color: transparent;
    border: none;
}

/* Language buttons - segmented control style */
QPushButton#langBtnLeft {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-right: none;
    border-top-left-radius: 3px;
    border-bottom-left-radius: 3px;
    border-top-right-radius: 0px;
    border-bottom-right-radius: 0px;
    font-size: 14px;
    font-weight: 500;
    color: #f5f5f7;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#langBtnLeft:hover {
    background-color: rgba(255, 255, 255, 0.06);
}

QPushButton#langBtnLeft:checked {
    background-color: #0a84ff;
    color: white;
}

QPushButton#langBtnRight {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-top-left-radius: 0px;
    border-bottom-left-radius: 0px;
    border-top-right-radius: 3px;
    border-bottom-right-radius: 3px;
    font-size: 14px;
    font-weight: 500;
    color: #f5f5f7;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#langBtnRight:hover {
    background-color: rgba(255, 255, 255, 0.06);
}

QPushButton#langBtnRight:checked {
    background-color: #0a84ff;
    color: white;
}

/* Edit Button (small icon button) */
QPushButton#editBtn {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    font-size: 14px;
    min-width: 28px;
    max-width: 28px;
    min-height: 28px;
    max-height: 28px;
    padding: 0px;
}

QPushButton#editBtn:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

QPushButton#editBtn:pressed {
    background-color: rgba(255, 255, 255, 0.08);
}

/* Toolbar icon buttons - Add/Remove segmented control */
QPushButton#addBtnLeft {
    background-color: #0a84ff;
    color: white;
    border: 1px solid #0a84ff;
    border-right: none;
    border-top-left-radius: 3px;
    border-bottom-left-radius: 3px;
    border-top-right-radius: 0px;
    border-bottom-right-radius: 0px;
    font-size: 16px;
    font-weight: bold;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#addBtnLeft:hover {
    background-color: #0070e0;
}

QPushButton#addRemoteBtnMiddle {
    background-color: #30d158;
    color: white;
    border: 1px solid #30d158;
    border-right: none;
    border-radius: 0px;
    font-size: 15px;
    font-weight: bold;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#addRemoteBtnMiddle:hover {
    background-color: #28b84d;
}

QPushButton#removeBtnRight {
    background-color: transparent;
    color: #ff453a;
    border: 1px solid #ff453a;
    border-top-left-radius: 0px;
    border-bottom-left-radius: 0px;
    border-top-right-radius: 3px;
    border-bottom-right-radius: 3px;
    font-size: 16px;
    font-weight: bold;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#removeBtnRight:hover {
    background-color: #ff453a;
    color: white;
}

QPushButton#swimlaneBtn {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    font-size: 16px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#swimlaneBtn:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

QPushButton#monitorBtn {
    background-color: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0px;
}

QPushButton#monitorBtn:hover {
    background-color: rgba(255, 255, 255, 0.05);
}
)")
        .arg(branchIndicatorStyle("#d0d0d4", "#ffffff"));
}

QString ThemeManager::currentStyle() const
{
    return m_currentTheme == Dark ? darkStyle() : lightStyle();
}
