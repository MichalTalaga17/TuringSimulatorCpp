#include "ThemeManager.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), m_currentTheme(Theme::Light)
{
    loadThemePreference();
    // Force apply the loaded theme (even if it's Light, we need to apply it initially)
    applyTheme(m_currentTheme);
}

void ThemeManager::applyTheme(Theme theme)
{
    // Apply palette
    qApp->setPalette(getPalette(theme));

    // Apply stylesheet
    qApp->setStyleSheet(getStylesheet(theme));
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme == theme)
        return;

    m_currentTheme = theme;
    saveThemePreference(theme);

    // Apply the theme
    applyTheme(theme);

    emit themeChanged(theme);
}

QString ThemeManager::getCurrentStylesheet() const
{
    return getStylesheet(m_currentTheme);
}

QString ThemeManager::getStylesheet(Theme theme)
{
    if (theme == Theme::Dark)
    {
        // Dark mode with mica effect
        return R"(
        QMainWindow {
            background-color: #1a1a1a;
        }
        QMenuBar {
            background-color: rgba(28, 28, 28, 0.95);
            color: #e0e0e0;
            border-bottom: 1px solid #2a2a2a;
            padding: 2px 6px;
            font-size: 12px;
        }
        QMenuBar::item {
            background: transparent;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QMenuBar::item:selected {
            background-color: rgba(100, 180, 255, 0.35);
            color: #7ec4ff;
        }
        QMenu {
            background-color: rgba(28, 28, 28, 0.98);
            color: #e0e0e0;
            border: 1px solid #2a2a2a;
            border-radius: 8px;
            padding: 6px;
        }
        QMenu::item {
            border-radius: 6px;
            padding: 7px 28px 7px 12px;
        }
        QMenu::item:selected {
            background-color: rgba(100, 180, 255, 0.35);
            color: #7ec4ff;
        }
        QMenu::separator {
            height: 1px;
            background-color: #2a2a2a;
            margin: 5px 6px;
        }
        QTabWidget::pane {
            border: none;
            background-color: #1a1a1a;
            top: 0;
        }
        QTabWidget::tab-bar {
            left: 8px;
        }
        QTabBar {
            background: rgba(26, 26, 26, 0.85);
        }
        QTabBar::base {
            border: none;
            height: 0;
        }
        QTabBar::tab {
            background-color: transparent;
            color: #a8a8a8;
            border: 1px solid transparent;
            border-radius: 8px;
            min-width: 96px;
            min-height: 22px;
            padding: 3px 30px 3px 12px;
            margin: 5px 3px 5px 0;
        }
        QTabBar::tab:selected {
            background-color: rgba(100, 180, 255, 0.25);
            color: #7ec4ff;
            border-color: rgba(100, 180, 255, 0.45);
            font-weight: 700;
        }
        QTabBar::tab:hover:!selected {
            background-color: rgba(100, 180, 255, 0.2);
            color: #a8a8a8;
            border-color: rgba(100, 180, 255, 0.35);
        }
        QPushButton#tabCloseButton {
            background-color: transparent;
            color: #808080;
            border: 1px solid transparent;
            border-radius: 6px;
            min-width: 14px;
            max-width: 14px;
            min-height: 14px;
            max-height: 14px;
            padding: 0;
            font-size: 10px;
            font-weight: 700;
        }
        QPushButton#tabCloseButton:hover {
            color: #7ec4ff;
        }
        QPushButton#tabCloseButton:pressed {
            color: #5ea4df;
        }
        QPushButton#tabAddButton {
            background-color: transparent;
            color: #7ec4ff;
            border: 1px solid transparent;
            border-radius: 8px;
            min-width: 24px;
            max-width: 24px;
            min-height: 24px;
            max-height: 24px;
            padding: 0;
            margin: 5px 10px 5px 4px;
            font-size: 18px;
            font-weight: 600;
        }
        QPushButton#tabAddButton:hover {
            color: #9fd4ff;
            background-color: rgba(100, 180, 255, 0.2);
            border-color: rgba(100, 180, 255, 0.4);
        }
        QPushButton#tabAddButton:pressed {
            color: #6eb4ff;
            background-color: rgba(100, 180, 255, 0.25);
            border-color: rgba(100, 180, 255, 0.5);
        }
        QFrame#tabContentFade {
            border: none;
            background: qlineargradient(
                x1: 0, y1: 0, x2: 0, y2: 1,
                stop: 0 rgba(40, 40, 40, 0.6),
                stop: 1 rgba(16, 16, 16, 0.9)
            );
        }
        QGroupBox {
            color: #d8d8d8;
            border: 1px solid #2a2a2a;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 8px;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 3px 0 3px;
            background-color: #1a1a1a;
        }
        QLineEdit {
            border: 1px solid #2a2a2a;
            border-radius: 6px;
            padding: 6px 8px;
            background-color: rgba(32, 32, 32, 0.85);
            color: #e8e8e8;
            selection-background-color: rgba(100, 180, 255, 0.5);
        }
        QLineEdit:focus {
            border: 2px solid #7ec4ff;
            padding: 5px 7px;
            background-color: rgba(35, 35, 35, 0.95);
        }
        QSpinBox {
            border: 1px solid #2a2a2a;
            border-radius: 6px;
            padding: 4px 6px;
            background-color: rgba(32, 32, 32, 0.85);
            color: #e8e8e8;
        }
        QSpinBox:focus {
            border: 2px solid #7ec4ff;
            background-color: rgba(35, 35, 35, 0.95);
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 0;
            border: none;
        }
        QComboBox {
            border: 1px solid #2a2a2a;
            border-radius: 6px;
            padding: 4px 8px;
            background-color: rgba(32, 32, 32, 0.85);
            color: #e8e8e8;
        }
        QComboBox:focus {
            border: 2px solid #7ec4ff;
            background-color: rgba(35, 35, 35, 0.95);
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border: none;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: url(:/arrow_down.png);
        }
        QComboBox QAbstractItemView {
            border: 1px solid #2a2a2a;
            border-radius: 6px;
            background-color: rgba(32, 32, 32, 0.98);
            color: #e8e8e8;
            selection-background-color: rgba(100, 180, 255, 0.4);
            selection-color: #7ec4ff;
            outline: 0;
        }
        QPushButton {
            background-color: rgba(100, 180, 255, 0.35);
            color: #7ec4ff;
            border: 1px solid rgba(100, 180, 255, 0.5);
            border-radius: 4px;
            padding: 6px 16px;
            font-weight: 600;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgba(100, 180, 255, 0.45);
            border: 1px solid rgba(100, 180, 255, 0.6);
        }
        QPushButton:pressed {
            background-color: rgba(100, 180, 255, 0.55);
            border: 1px solid rgba(100, 180, 255, 0.7);
        }
        QPushButton:disabled {
            background-color: rgba(100, 100, 100, 0.25);
            color: #707070;
            border: 1px solid rgba(100, 100, 100, 0.4);
        }
        QStatusBar QPushButton {
            background-color: rgba(32, 32, 32, 0.85);
            color: #e8e8e8;
            border: 1px solid rgba(100, 180, 255, 0.4);
            border-radius: 8px;
            padding: 4px 10px;
            font-size: 11px;
            font-weight: 700;
            min-width: 58px;
            min-height: 28px;
        }
        QStatusBar QPushButton:hover {
            background-color: rgba(45, 45, 45, 0.95);
            border: 1px solid rgba(100, 180, 255, 0.6);
            color: #ffffff;
        }
        QStatusBar QPushButton:pressed {
            background-color: rgba(50, 50, 50, 0.95);
            border: 1px solid rgba(100, 180, 255, 0.7);
            color: #ffffff;
        }
        QStatusBar QPushButton:disabled {
            background-color: rgba(30, 30, 30, 0.85);
            border: 1px solid rgba(100, 100, 100, 0.3);
            color: #606060;
        }
        QTableWidget#transitionsTable {
            border: 1px solid #2a2a2a;
            border-radius: 8px;
            background-color: rgba(32, 32, 32, 0.95);
            alternate-background-color: rgba(28, 28, 28, 0.95);
            gridline-color: #252525;
            outline: 0;
            selection-background-color: transparent;
        }
        QTableWidget#transitionsTable::viewport {
            background-color: rgba(32, 32, 32, 0.95);
            border-radius: 8px;
        }
        QTableWidget#transitionsTable::item {
            padding: 4px;
            border: none;
            background-color: transparent;
            color: #e8e8e8;
        }
        QTableWidget#transitionsTable::item:selected {
            background-color: rgba(100, 180, 255, 0.3);
            color: #ffffff;
        }
        QTableWidget#transitionsTable QHeaderView {
            background-color: rgba(45, 45, 45, 0.9);
        }
        QTableWidget#transitionsTable QHeaderView::section {
            background-color: rgba(45, 45, 45, 0.9);
            color: #c0c0c0;
            padding: 7px 8px;
            border: none;
            border-right: 1px solid #2a2a2a;
            border-bottom: 1px solid #2a2a2a;
            font-size: 11px;
            font-weight: 700;
        }
        QTableCornerButton::section {
            background-color: rgba(45, 45, 45, 0.9);
            border: none;
            border-right: 1px solid #2a2a2a;
            border-bottom: 1px solid #2a2a2a;
        }
        QTableWidget#transitionsTable QScrollBar:vertical,
        QTableWidget#transitionsTable QScrollBar:horizontal {
            background-color: rgba(45, 45, 45, 0.9);
            border: none;
            margin: 0;
        }
        QTableWidget#transitionsTable QScrollBar:vertical {
            width: 10px;
        }
        QTableWidget#transitionsTable QScrollBar:horizontal {
            height: 10px;
        }
        QTableWidget#transitionsTable QScrollBar::handle:vertical,
        QTableWidget#transitionsTable QScrollBar::handle:horizontal {
            background-color: rgba(100, 180, 255, 0.5);
            border-radius: 5px;
            min-height: 28px;
            min-width: 28px;
        }
        QTableWidget#transitionsTable QScrollBar::handle:vertical:hover,
        QTableWidget#transitionsTable QScrollBar::handle:horizontal:hover {
            background-color: rgba(100, 180, 255, 0.6);
        }
        QTableWidget#transitionsTable QScrollBar::add-line,
        QTableWidget#transitionsTable QScrollBar::sub-line,
        QTableWidget#transitionsTable QScrollBar::add-page,
        QTableWidget#transitionsTable QScrollBar::sub-page {
            background: transparent;
            border: none;
            width: 0;
            height: 0;
        }
        QLabel {
            color: #d8d8d8;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 4px;
            background: rgba(100, 100, 100, 0.6);
            margin: 7px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal {
            background: rgba(100, 180, 255, 0.7);
            border: 1px solid rgba(100, 180, 255, 0.9);
            width: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            border-color: rgba(100, 180, 255, 1.0);
            background: rgba(100, 180, 255, 0.8);
        }
        QSlider::handle:horizontal:pressed {
            border-color: rgba(100, 180, 255, 1.0);
            background: rgba(100, 180, 255, 0.9);
        }
        QSlider::sub-page:horizontal {
            background: rgba(100, 180, 255, 0.5);
            border-radius: 8px;
        }
        QSlider::add-page:horizontal {
            background: rgba(100, 100, 100, 0.4);
            border-radius: 8px;
        }
        QScrollArea {
            background-color: #1a1a1a;
            border: none;
        }
    )";
    }
    else
    {
        // Light mode with subtle mica effect
        return R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        QMenuBar {
            background-color: rgba(255, 255, 255, 0.9);
            color: #202020;
            border-bottom: 1px solid #e0e0e0;
            padding: 2px 6px;
            font-size: 12px;
        }
        QMenuBar::item {
            background: transparent;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QMenuBar::item:selected {
            background-color: rgba(0, 120, 212, 0.15);
            color: #005a9e;
        }
        QMenu {
            background-color: rgba(255, 255, 255, 0.98);
            color: #202020;
            border: 1px solid #d0d0d0;
            border-radius: 8px;
            padding: 6px;
        }
        QMenu::item {
            border-radius: 6px;
            padding: 7px 28px 7px 12px;
        }
        QMenu::item:selected {
            background-color: rgba(0, 120, 212, 0.15);
            color: #005a9e;
        }
        QMenu::separator {
            height: 1px;
            background-color: #eeeeee;
            margin: 5px 6px;
        }
        QTabWidget::pane {
            border: none;
            background-color: #f5f5f5;
            top: 0;
        }
        QTabWidget::tab-bar {
            left: 8px;
        }
        QTabBar {
            background: rgba(250, 250, 250, 0.8);
        }
        QTabBar::base {
            border: none;
            height: 0;
        }
        QTabBar::tab {
            background-color: transparent;
            color: #505050;
            border: 1px solid transparent;
            border-radius: 8px;
            min-width: 96px;
            min-height: 22px;
            padding: 3px 30px 3px 12px;
            margin: 5px 3px 5px 0;
        }
        QTabBar::tab:selected {
            background-color: rgba(0, 120, 212, 0.15);
            color: #174ea6;
            border-color: rgba(0, 120, 212, 0.3);
            font-weight: 700;
        }
        QTabBar::tab:hover:!selected {
            background-color: rgba(0, 120, 212, 0.1);
            color: #505050;
            border-color: rgba(0, 120, 212, 0.2);
        }
        QPushButton#tabCloseButton {
            background-color: transparent;
            color: #667085;
            border: 1px solid transparent;
            border-radius: 6px;
            min-width: 14px;
            max-width: 14px;
            min-height: 14px;
            max-height: 14px;
            padding: 0;
            font-size: 10px;
            font-weight: 700;
        }
        QPushButton#tabCloseButton:hover {
            color: #174ea6;
        }
        QPushButton#tabCloseButton:pressed {
            color: #0f3f8f;
        }
        QPushButton#tabAddButton {
            background-color: transparent;
            color: #174ea6;
            border: 1px solid transparent;
            border-radius: 8px;
            min-width: 24px;
            max-width: 24px;
            min-height: 24px;
            max-height: 24px;
            padding: 0;
            margin: 5px 10px 5px 4px;
            font-size: 18px;
            font-weight: 600;
        }
        QPushButton#tabAddButton:hover {
            color: #0f3f8f;
            background-color: rgba(0, 120, 212, 0.15);
            border-color: rgba(0, 120, 212, 0.3);
        }
        QPushButton#tabAddButton:pressed {
            color: #0f3f8f;
            background-color: rgba(0, 120, 212, 0.2);
            border-color: rgba(0, 120, 212, 0.4);
        }
        QFrame#tabContentFade {
            border: none;
            background: qlineargradient(
                x1: 0, y1: 0, x2: 0, y2: 1,
                stop: 0 rgba(255, 255, 255, 0.6),
                stop: 1 rgba(245, 245, 245, 0.9)
            );
        }
        QGroupBox {
            color: #202020;
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 8px;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 3px 0 3px;
            background-color: #f5f5f5;
        }
        QLineEdit {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 6px 8px;
            background-color: rgba(255, 255, 255, 0.9);
            color: #202020;
            selection-background-color: rgba(0, 120, 212, 0.3);
        }
        QLineEdit:focus {
            border: 2px solid #0078d4;
            padding: 5px 7px;
            background-color: #ffffff;
        }
        QSpinBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 4px 6px;
            background-color: rgba(255, 255, 255, 0.9);
            color: #202020;
        }
        QSpinBox:focus {
            border: 2px solid #0078d4;
            background-color: #ffffff;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 0;
            border: none;
        }
        QComboBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 4px 8px;
            background-color: rgba(255, 255, 255, 0.9);
            color: #202020;
        }
        QComboBox:focus {
            border: 2px solid #0078d4;
            background-color: #ffffff;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border: none;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: url(:/arrow_down.png);
        }
        QComboBox QAbstractItemView {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            background-color: rgba(255, 255, 255, 0.98);
            color: #202020;
            selection-background-color: rgba(0, 120, 212, 0.2);
            selection-color: #005a9e;
            outline: 0;
        }
        QPushButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            font-weight: 600;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #106ebe;
        }
        QPushButton:pressed {
            background-color: #005a9e;
        }
        QPushButton:disabled {
            background-color: #e0e0e0;
            color: #9e9e9e;
        }
        QStatusBar QPushButton {
            background-color: #ffffff;
            color: #202020;
            border: 1px solid #d0d0d0;
            border-radius: 8px;
            padding: 4px 10px;
            font-size: 11px;
            font-weight: 700;
            min-width: 58px;
            min-height: 28px;
        }
        QStatusBar QPushButton:hover {
            background-color: #eaf4ff;
            border: 1px solid #0078d4;
            color: #005a9e;
        }
        QStatusBar QPushButton:pressed {
            background-color: #d8ecff;
            border: 1px solid #005a9e;
            color: #005a9e;
        }
        QStatusBar QPushButton:disabled {
            background-color: #f0f0f0;
            border: 1px solid #dddddd;
            color: #888888;
        }
        QTableWidget#transitionsTable {
            border: 1px solid #d0d0d0;
            border-radius: 8px;
            background-color: rgba(255, 255, 255, 0.95);
            alternate-background-color: rgba(250, 250, 250, 0.95);
            gridline-color: #ececec;
            outline: 0;
            selection-background-color: transparent;
        }
        QTableWidget#transitionsTable::viewport {
            background-color: rgba(255, 255, 255, 0.95);
            border-radius: 8px;
        }
        QTableWidget#transitionsTable::item {
            padding: 4px;
            border: none;
            background-color: transparent;
            color: #202020;
        }
        QTableWidget#transitionsTable::item:selected {
            background-color: rgba(0, 120, 212, 0.15);
            color: #005a9e;
        }
        QTableWidget#transitionsTable QHeaderView {
            background-color: #f7f7f7;
        }
        QTableWidget#transitionsTable QHeaderView::section {
            background-color: #f7f7f7;
            color: #505050;
            padding: 7px 8px;
            border: none;
            border-right: 1px solid #e5e5e5;
            border-bottom: 1px solid #e5e5e5;
            font-size: 11px;
            font-weight: 700;
        }
        QTableCornerButton::section {
            background-color: #f7f7f7;
            border: none;
            border-right: 1px solid #e5e5e5;
            border-bottom: 1px solid #e5e5e5;
        }
        QTableWidget#transitionsTable QScrollBar:vertical,
        QTableWidget#transitionsTable QScrollBar:horizontal {
            background-color: #f7f7f7;
            border: none;
            margin: 0;
        }
        QTableWidget#transitionsTable QScrollBar:vertical {
            width: 10px;
        }
        QTableWidget#transitionsTable QScrollBar:horizontal {
            height: 10px;
        }
        QTableWidget#transitionsTable QScrollBar::handle:vertical,
        QTableWidget#transitionsTable QScrollBar::handle:horizontal {
            background-color: #cfcfcf;
            border-radius: 5px;
            min-height: 28px;
            min-width: 28px;
        }
        QTableWidget#transitionsTable QScrollBar::handle:vertical:hover,
        QTableWidget#transitionsTable QScrollBar::handle:horizontal:hover {
            background-color: #a8c7e8;
        }
        QTableWidget#transitionsTable QScrollBar::add-line,
        QTableWidget#transitionsTable QScrollBar::sub-line,
        QTableWidget#transitionsTable QScrollBar::add-page,
        QTableWidget#transitionsTable QScrollBar::sub-page {
            background: transparent;
            border: none;
            width: 0;
            height: 0;
        }
        QLabel {
            color: #202020;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 4px;
            background: #dedede;
            margin: 7px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal {
            background: #ffffff;
            border: 1px solid #eeeeee;
            width: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            border-color: #d7d7d7;
        }
        QSlider::handle:horizontal:pressed {
            border-color: #c9c9c9;
        }
        QSlider::sub-page:horizontal {
            background: #0078d4;
            border-radius: 8px;
        }
        QSlider::add-page:horizontal {
            background: #dedede;
            border-radius: 8px;
        }
        QScrollArea {
            background-color: #f5f5f5;
            border: none;
        }
    )";
    }
}

QPalette ThemeManager::getPalette(Theme theme)
{
    QPalette palette;

    if (theme == Theme::Dark)
    {
        palette.setColor(QPalette::Window, QColor(31, 31, 31));
        palette.setColor(QPalette::WindowText, QColor(224, 224, 224));
        palette.setColor(QPalette::Base, QColor(40, 40, 40));
        palette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
        palette.setColor(QPalette::ToolTipBase, QColor(50, 50, 50));
        palette.setColor(QPalette::ToolTipText, QColor(224, 224, 224));
        palette.setColor(QPalette::Text, QColor(224, 224, 224));
        palette.setColor(QPalette::Button, QColor(40, 40, 40));
        palette.setColor(QPalette::ButtonText, QColor(224, 224, 224));
        palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
        palette.setColor(QPalette::Link, QColor(100, 180, 255));
        palette.setColor(QPalette::Highlight, QColor(100, 180, 255));
        palette.setColor(QPalette::HighlightedText, QColor(31, 31, 31));
    }
    else
    {
        palette.setColor(QPalette::Window, QColor(245, 245, 245));
        palette.setColor(QPalette::WindowText, QColor(32, 32, 32));
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
        palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
        palette.setColor(QPalette::ToolTipText, QColor(32, 32, 32));
        palette.setColor(QPalette::Text, QColor(32, 32, 32));
        palette.setColor(QPalette::Button, QColor(243, 243, 243));
        palette.setColor(QPalette::ButtonText, QColor(32, 32, 32));
        palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
        palette.setColor(QPalette::Link, QColor(0, 120, 212));
        palette.setColor(QPalette::Highlight, QColor(0, 120, 212));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    }

    return palette;
}

void ThemeManager::loadThemePreference()
{
    QSettings settings;
    QString themeName = settings.value("theme", "Light").toString();
    m_currentTheme = (themeName == "Dark") ? Theme::Dark : Theme::Light;
}

void ThemeManager::saveThemePreference(Theme theme)
{
    QSettings settings;
    settings.setValue("theme", (theme == Theme::Dark) ? "Dark" : "Light");
}
