#include "TuringSimulatorUI.h"

#include <QApplication>
#include <QColor>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleFactory>
#include <QTabBar>
#include <QInputDialog>
#include <algorithm>

TuringSimulatorUI::TuringSimulatorUI(QWidget *parent)
    : QMainWindow(parent),
      machineTabs(nullptr),
      newTabButton(nullptr),
      saveAction(nullptr),
      saveAsAction(nullptr),
      closeTabAction(nullptr),
      validateAction(nullptr),
      resetExampleAction(nullptr),
      stepAction(nullptr),
      runAction(nullptr),
      resetAction(nullptr),
      previousTabAction(nullptr),
      nextTabAction(nullptr),
      untitledTabCounter(1)
{
    setWindowTitle("Turing Machine Simulator");
    setGeometry(100, 100, 1200, 850);
    setMinimumWidth(980);
    setMinimumHeight(720);

    setupMenu();
    setupUI();
    createExampleMachineTab();
    updateMenuState();
}

void TuringSimulatorUI::setupUI()
{
    // Apply modern style once for entire application
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(243, 243, 243));
    darkPalette.setColor(QPalette::WindowText, QColor(32, 32, 32));
    darkPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::ToolTipText, QColor(32, 32, 32));
    darkPalette.setColor(QPalette::Text, QColor(32, 32, 32));
    darkPalette.setColor(QPalette::Button, QColor(243, 243, 243));
    darkPalette.setColor(QPalette::ButtonText, QColor(32, 32, 32));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Link, QColor(0, 120, 212));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 120, 212));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));

    qApp->setPalette(darkPalette);

    QString styleSheet = R"(
        QMainWindow {
            background-color: #f3f3f3;
        }
        QMenuBar {
            background-color: #ffffff;
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
            background-color: #eaf4ff;
            color: #005a9e;
        }
        QMenu {
            background-color: #ffffff;
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
            background-color: #eaf4ff;
            color: #005a9e;
        }
        QMenu::separator {
            height: 1px;
            background-color: #eeeeee;
            margin: 5px 6px;
        }
        QTabWidget::pane {
            border: none;
            background-color: #f3f3f3;
            top: 0;
        }
        QTabWidget::tab-bar {
            left: 8px;
        }
        QTabBar {
            background: qlineargradient(
                x1: 0, y1: 0, x2: 0, y2: 1,
                stop: 0 #f7f9fc,
                stop: 1 #edf3fb
            );
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
            background-color: #dfeaff;
            color: #174ea6;
            border-color: #3f74d8;
            font-weight: 700;
        }
        QTabBar::tab:hover:!selected {
            background-color: #eaf2ff;
            color: #174ea6;
            border-color: #b8ccf2;
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
            background-color: #eaf2ff;
            border-color: #b8ccf2;
        }
        QPushButton#tabAddButton:pressed {
            color: #0f3f8f;
            background-color: #dfeaff;
            border-color: #3f74d8;
        }
        QFrame#tabContentFade {
            border: none;
            background: qlineargradient(
                x1: 0, y1: 0, x2: 0, y2: 1,
                stop: 0 #ffffff,
                stop: 1 #f3f3f3
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
            background-color: #f3f3f3;
        }
        QLineEdit {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 6px 8px;
            background-color: #ffffff;
            color: #202020;
            selection-background-color: #0078d4;
        }
        QLineEdit:focus {
            border: 2px solid #0078d4;
            padding: 5px 7px;
        }
        QSpinBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 4px 6px;
            background-color: #ffffff;
            color: #202020;
        }
        QSpinBox:focus {
            border: 2px solid #0078d4;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 0;
            border: none;
        }
        QComboBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 4px 8px;
            background-color: #ffffff;
            color: #202020;
        }
        QComboBox:focus {
            border: 2px solid #0078d4;
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
            background-color: #ffffff;
            color: #202020;
            selection-background-color: #0078d4;
            selection-color: #ffffff;
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
        QTableWidget#transitionsTable {
            border: 1px solid #d0d0d0;
            border-radius: 8px;
            background-color: #ffffff;
            alternate-background-color: #fbfbfb;
            gridline-color: #ececec;
            outline: 0;
            selection-background-color: transparent;
        }
        QTableWidget#transitionsTable::viewport {
            background-color: #ffffff;
            border-radius: 8px;
        }
        QTableWidget#transitionsTable::item {
            padding: 4px;
            border: none;
            background-color: transparent;
        }
        QTableWidget#transitionsTable::item:selected {
            background-color: #eaf4ff;
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
    )";

    qApp->setStyleSheet(styleSheet);

    machineTabs = new QTabWidget(this);
    machineTabs->setDocumentMode(true);
    machineTabs->setMovable(true);
    machineTabs->setTabsClosable(false);
    machineTabs->setElideMode(Qt::ElideRight);
    machineTabs->tabBar()->setDrawBase(false);
    machineTabs->tabBar()->setExpanding(false);

    connect(machineTabs, &QTabWidget::currentChanged, this, [this](int)
            {
        updateMenuState();
        updateNewTabButtonPosition(); });
    connect(machineTabs->tabBar(), &QTabBar::tabMoved, this, [this](int, int)
            { updateNewTabButtonPosition(); });

    newTabButton = new QPushButton("+", machineTabs);
    newTabButton->setObjectName("tabAddButton");
    newTabButton->setCursor(Qt::PointingHandCursor);
    newTabButton->setFocusPolicy(Qt::NoFocus);
    newTabButton->setToolTip("Nowa maszyna");
    connect(newTabButton, &QPushButton::clicked, this, &TuringSimulatorUI::showNewTabMenu);
    machineTabs->setCornerWidget(newTabButton, Qt::TopRightCorner);

    setCentralWidget(machineTabs);
}

void TuringSimulatorUI::setupMenu()
{
    menuBar()->setNativeMenuBar(false);

    auto bindShortcut = [this](QAction *action, const QKeySequence &shortcut)
    {
        action->setShortcut(shortcut);
        action->setShortcutContext(Qt::ApplicationShortcut);
        addAction(action);
    };

    QMenu *fileMenu = menuBar()->addMenu("Plik");
    QAction *newTabAction = fileMenu->addAction("Zacznij od zera");
    bindShortcut(newTabAction, QKeySequence(QKeySequence::New));
    connect(newTabAction, &QAction::triggered, this, &TuringSimulatorUI::createNewMachineTab);

    QAction *exampleTabAction = fileMenu->addAction("Zacznij od przykładu");
    connect(exampleTabAction, &QAction::triggered, this, &TuringSimulatorUI::createExampleMachineTab);

    QAction *openAction = fileMenu->addAction("Otwórz w nowej karcie...");
    bindShortcut(openAction, QKeySequence(QKeySequence::Open));
    connect(openAction, &QAction::triggered, this, &TuringSimulatorUI::openMachineInNewTab);

    fileMenu->addSeparator();

    saveAction = fileMenu->addAction("Zapisz");
    bindShortcut(saveAction, QKeySequence(QKeySequence::Save));
    connect(saveAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachine);

    saveAsAction = fileMenu->addAction("Zapisz jako...");
    bindShortcut(saveAsAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachineAs);

    fileMenu->addSeparator();

    QAction *openDbAction = fileMenu->addAction("Wczytaj z bazy danych...");
    connect(openDbAction, &QAction::triggered, this, &TuringSimulatorUI::openMachineFromDatabase);


    QAction *saveDbAction = fileMenu->addAction("Zapisz w bazie danych...");
    connect(saveDbAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachineToDatabase);

    fileMenu->addSeparator();

    closeTabAction = fileMenu->addAction("Zamknij kartę");
    bindShortcut(closeTabAction, QKeySequence(QKeySequence::Close));
    connect(closeTabAction, &QAction::triggered, this, &TuringSimulatorUI::closeCurrentMachineTab);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction("Zakończ");
    bindShortcut(quitAction, QKeySequence(QKeySequence::Quit));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *machineMenu = menuBar()->addMenu("Maszyna");
    validateAction = machineMenu->addAction("Waliduj i przebuduj");
    bindShortcut(validateAction, QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(validateAction, &QAction::triggered, this, &TuringSimulatorUI::validateCurrentMachine);

    resetExampleAction = machineMenu->addAction("Przywróć przykład startowy");
    connect(resetExampleAction, &QAction::triggered, this, &TuringSimulatorUI::resetCurrentMachineToExample);

    QMenu *simulationMenu = menuBar()->addMenu("Symulacja");
    stepAction = simulationMenu->addAction("Krok");
    bindShortcut(stepAction, QKeySequence(Qt::Key_F10));
    connect(stepAction, &QAction::triggered, this, &TuringSimulatorUI::stepCurrentMachine);

    runAction = simulationMenu->addAction("Uruchom / pauza");
    bindShortcut(runAction, QKeySequence(Qt::Key_F5));
    connect(runAction, &QAction::triggered, this, &TuringSimulatorUI::toggleCurrentMachineRun);

    resetAction = simulationMenu->addAction("Reset");
    bindShortcut(resetAction, QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(resetAction, &QAction::triggered, this, &TuringSimulatorUI::resetCurrentMachine);

    QMenu *viewMenu = menuBar()->addMenu("Widok");
    previousTabAction = viewMenu->addAction("Poprzednia karta");
    bindShortcut(previousTabAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(previousTabAction, &QAction::triggered, this, &TuringSimulatorUI::selectPreviousTab);

    nextTabAction = viewMenu->addAction("Następna karta");
    bindShortcut(nextTabAction, QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(nextTabAction, &QAction::triggered, this, &TuringSimulatorUI::selectNextTab);

    QMenu *helpMenu = menuBar()->addMenu("Pomoc");
    QAction *aboutAction = helpMenu->addAction("O programie");
    connect(aboutAction, &QAction::triggered, this, &TuringSimulatorUI::showAboutDialog);
}

MachineEditorTab *TuringSimulatorUI::currentMachineTab() const
{
    if (!machineTabs)
    {
        return nullptr;
    }

    return qobject_cast<MachineEditorTab *>(machineTabs->currentWidget());
}

void TuringSimulatorUI::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateNewTabButtonPosition();
}

void TuringSimulatorUI::updateNewTabButtonPosition()
{
    if (!newTabButton)
    {
        return;
    }

    newTabButton->raise();
    newTabButton->show();
}

void TuringSimulatorUI::showNewTabMenu()
{
    if (!newTabButton)
    {
        return;
    }

    QMenu menu(this);
    QAction *blankAction = menu.addAction("Zacznij od zera");
    QAction *exampleAction = menu.addAction("Zacznij od przykładu");
    QAction *aiAction = menu.addAction("Zacznij z AI");
    aiAction->setEnabled(false);

    connect(blankAction, &QAction::triggered, this, &TuringSimulatorUI::createNewMachineTab);
    connect(exampleAction, &QAction::triggered, this, &TuringSimulatorUI::createExampleMachineTab);

    const QPoint menuPos = newTabButton->mapToGlobal(QPoint(0, newTabButton->height()));
    menu.exec(menuPos);
}

void TuringSimulatorUI::addMachineTab(MachineEditorTab *tab)
{
    const int index = machineTabs->addTab(tab, tab->tabTitle());
    machineTabs->setCurrentIndex(index);
    updateNewTabButtonPosition();

    QWidget *closeButtonWrapper = new QWidget(machineTabs);
    QHBoxLayout *closeButtonLayout = new QHBoxLayout(closeButtonWrapper);
    closeButtonLayout->setContentsMargins(0, 0, 6, 0);
    closeButtonLayout->setSpacing(0);

    QPushButton *closeButton = new QPushButton("x", closeButtonWrapper);
    closeButton->setObjectName("tabCloseButton");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFocusPolicy(Qt::NoFocus);
    closeButton->setToolTip("Zamknij kartę");
    closeButtonLayout->addWidget(closeButton);
    machineTabs->tabBar()->setTabButton(index, QTabBar::RightSide, closeButtonWrapper);

    connect(closeButton, &QPushButton::clicked, this, [this, tab]()
            {
        const int tabIndex = machineTabs->indexOf(tab);
        if (tabIndex >= 0)
        {
            machineTabs->setCurrentIndex(tabIndex);
            closeCurrentMachineTab();
        } });

    connect(tab, &MachineEditorTab::titleChanged, this, [this, tab](const QString &title)
            {
        const int tabIndex = machineTabs->indexOf(tab);
        if (tabIndex >= 0)
        {
            machineTabs->setTabText(tabIndex, title);
        }
        updateMenuState();
        updateNewTabButtonPosition(); });

    updateMenuState();
    updateNewTabButtonPosition();
}

void TuringSimulatorUI::createNewMachineTab()
{
    MachineEditorTab *tab = new MachineEditorTab(QString("Maszyna %1").arg(untitledTabCounter++), false, machineTabs);
    addMachineTab(tab);
}

void TuringSimulatorUI::createExampleMachineTab()
{
    MachineEditorTab *tab = new MachineEditorTab(QString("Maszyna %1").arg(untitledTabCounter++), true, machineTabs);
    addMachineTab(tab);
}

void TuringSimulatorUI::openMachineInNewTab()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "Wczytaj maszynę", "", "JSON Files (*.json)");
    if (filePath.isEmpty())
    {
        return;
    }

    MachineEditorTab *tab = new MachineEditorTab(QString("Maszyna %1").arg(untitledTabCounter++), false, machineTabs);
    if (!tab->loadFromFile(filePath))
    {
        tab->deleteLater();
        return;
    }

    addMachineTab(tab);
}

void TuringSimulatorUI::saveCurrentMachine()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->saveToFile();
    }
}

void TuringSimulatorUI::saveCurrentMachineAs()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->saveToFileAs();
    }
}

void TuringSimulatorUI::closeCurrentMachineTab()
{
    MachineEditorTab *currentTab = currentMachineTab();
    if (!currentTab)
    {
        return;
    }

    const int index = machineTabs->indexOf(currentTab);
    if (index < 0)
    {
        return;
    }

    machineTabs->removeTab(index);
    currentTab->deleteLater();

    bool hasMachineTab = false;
    for (int tabIndex = 0; tabIndex < machineTabs->count(); ++tabIndex)
    {
        if (qobject_cast<MachineEditorTab *>(machineTabs->widget(tabIndex)))
        {
            hasMachineTab = true;
            break;
        }
    }

    if (!hasMachineTab)
    {
        createExampleMachineTab();
    }

    updateMenuState();
    updateNewTabButtonPosition();
}

void TuringSimulatorUI::validateCurrentMachine()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->validateMachine();
    }
}

void TuringSimulatorUI::resetCurrentMachineToExample()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->resetToDefaultExample();
    }
}

void TuringSimulatorUI::stepCurrentMachine()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->onStepClicked();
    }
}

void TuringSimulatorUI::toggleCurrentMachineRun()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->onRunClicked();
    }
}

void TuringSimulatorUI::resetCurrentMachine()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->onResetClicked();
    }
}

void TuringSimulatorUI::selectPreviousTab()
{
    const int count = machineTabs->count();
    if (count <= 1)
    {
        return;
    }

    const int nextIndex = (machineTabs->currentIndex() - 1 + count) % count;
    machineTabs->setCurrentIndex(nextIndex);
}

void TuringSimulatorUI::selectNextTab()
{
    const int count = machineTabs->count();
    if (count <= 1)
    {
        return;
    }

    const int nextIndex = (machineTabs->currentIndex() + 1) % count;
    machineTabs->setCurrentIndex(nextIndex);
}

void TuringSimulatorUI::updateMenuState()
{
    const bool hasTab = currentMachineTab() != nullptr;
    const bool hasMultipleTabs = machineTabs && machineTabs->count() > 1;

    saveAction->setEnabled(hasTab);
    saveAsAction->setEnabled(hasTab);
    closeTabAction->setEnabled(hasTab);
    validateAction->setEnabled(hasTab);
    resetExampleAction->setEnabled(hasTab);
    stepAction->setEnabled(hasTab);
    runAction->setEnabled(hasTab);
    resetAction->setEnabled(hasTab);
    previousTabAction->setEnabled(hasMultipleTabs);
    nextTabAction->setEnabled(hasMultipleTabs);

    QString title = "Turing Machine Simulator";
    if (MachineEditorTab *tab = currentMachineTab())
    {
        title += " - " + tab->tabTitle();
    }
    setWindowTitle(title);
}

void TuringSimulatorUI::showAboutDialog()
{
    QMessageBox::about(
        this,
        "O programie",
        "Symulator maszyny Turinga\n\n"
        "Edytuj definicję maszyny, otwieraj wiele plików w osobnych kartach "
        "i uruchamiaj symulację bez przełączania okien.");
}

void TuringSimulatorUI::openMachineFromDatabase()
{
    // 1. Pobieramy listę dostępnych nazw maszyn z bazy
    QStringList names = getMachineNamesFromDatabase();
    if (names.isEmpty()) {
        QMessageBox::information(this, "Baza danych", "Baza danych jest pusta. Zapisz najpierw jakąś maszynę.");
        return;
    }

    // 2. Wyświetlamy okno wyboru
    bool ok;
    QString chosenName = QInputDialog::getItem(this, "Wczytaj z bazy", "Wybierz maszynę:", names, 0, false, &ok);

    if (ok && !chosenName.isEmpty()) {
        // 3. Tworzymy NOWĄ zakładkę (zamiast używać currentMachineTab())
        MachineEditorTab *tab = new MachineEditorTab(chosenName, false, machineTabs);

        // 4. Delegujemy wczytanie do nowej zakładki (używając wersji z parametrem)
        if (!tab->loadFromDatabase(chosenName)) {
            tab->deleteLater(); // Jeśli wczytanie się nie udało, czyścimy
            return;
        }

        // 5. Dodajemy nową zakładkę do interfejsu
        addMachineTab(tab);
    }
}

void TuringSimulatorUI::saveCurrentMachineToDatabase()
{
    if (MachineEditorTab *tab = currentMachineTab()) {
        tab->saveToDatabase();
    }
}
