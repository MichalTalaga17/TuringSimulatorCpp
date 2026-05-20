#include "TuringSimulatorUI.h"

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
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
