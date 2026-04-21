#include "stylemanager.h"
#include <QFile>

StyleManager& StyleManager::instance()
{
    static StyleManager manager;
    return manager;
}

QString StyleManager::modernStyle() const
{
    return R"(
/* Global Settings */
QWidget {
    font-family: "Helvetica Neue", Arial, sans-serif;
    font-size: 14px;
    color: #1d1d1f;
}

/* Main Window */
QMainWindow {
    background-color: #f5f5f7;
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
)" + menuStyle() + buttonStyle() + listStyle() + scrollBarStyle();
}

QString StyleManager::menuStyle() const
{
    return R"(
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
)";
}

QString StyleManager::buttonStyle() const
{
    return R"(
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
)";
}

QString StyleManager::listStyle() const
{
    return R"(
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

/* Text Edit / Preview Pane */
QTextEdit {
    background-color: white;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 10px;
    padding: 16px;
    font-size: 14px;
    line-height: 1.5;
    selection-background-color: rgba(0, 122, 255, 0.3);
}

QTextEdit:focus {
    border-color: rgba(0, 122, 255, 0.5);
}
)";
}

QString StyleManager::scrollBarStyle() const
{
    return R"(
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
)";
}

QString StyleManager::loadStyleFromFile(const QString& filename) const
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return QString();
}
