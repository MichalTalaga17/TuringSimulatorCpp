#include "TuringSimulatorUI.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyleFactory>
#include <QTabBar>
#include <QVBoxLayout>
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
      toggleThemeAction(nullptr),
      untitledTabCounter(1),
      themeManager(std::make_unique<ThemeManager>())
{
    setWindowTitle("Turing Machine Simulator");
    setGeometry(100, 100, 1200, 850);
    setMinimumWidth(980);
    setMinimumHeight(720);

    setupMenu();
    setupUI();
    showStartPage();
    updateMenuState();
}

void TuringSimulatorUI::setupUI()
{
    // Apply Fusion style
    qApp->setStyle(QStyleFactory::create("Fusion"));

    // Theme has already been initialized in constructor via themeManager
    // No need to call setTheme again as it's already applied

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
    connect(newTabButton, &QPushButton::clicked, this, &TuringSimulatorUI::showStartPage);
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

    QAction *openAction = fileMenu->addAction("Wczytaj maszynę z pliku .json");
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

    viewMenu->addSeparator();

    toggleThemeAction = viewMenu->addAction("Tryb ciemny");
    bindShortcut(toggleThemeAction, QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(toggleThemeAction, &QAction::triggered, this, &TuringSimulatorUI::toggleTheme);

    QMenu *helpMenu = menuBar()->addMenu("Pomoc");
    QAction *aboutAction = helpMenu->addAction("O programie");
    connect(aboutAction, &QAction::triggered, this, &TuringSimulatorUI::showAboutDialog);

    QAction* aboutAuthorsAction = helpMenu->addAction("O autorach");
    connect(aboutAuthorsAction, &QAction::triggered, this, &TuringSimulatorUI::showAboutAuthorsDialog);
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

QWidget *TuringSimulatorUI::createStartPage()
{
    QWidget *page = new QWidget(machineTabs);
    page->setObjectName("startPage");
    page->setFont(qApp->font());

    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *tabContentFade = new QFrame(page);
    tabContentFade->setObjectName("tabContentFade");
    tabContentFade->setFrameShape(QFrame::NoFrame);
    tabContentFade->setFixedHeight(8);
    mainLayout->addWidget(tabContentFade);

    QScrollArea *pageScrollArea = new QScrollArea(page);
    pageScrollArea->setWidgetResizable(true);
    pageScrollArea->setFrameShape(QFrame::NoFrame);
    pageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *pageWidget = new QWidget(pageScrollArea);
    QVBoxLayout *pageLayout = new QVBoxLayout(pageWidget);
    pageLayout->setContentsMargins(24, 24, 24, 24);
    pageLayout->setSpacing(12);

    QWidget *sectionsWrapper = new QWidget(pageWidget);
    QHBoxLayout *sectionsWrapperLayout = new QHBoxLayout(sectionsWrapper);
    sectionsWrapperLayout->setContentsMargins(0, 0, 0, 0);
    sectionsWrapperLayout->setSpacing(0);

    QWidget *sectionsWidget = new QWidget(sectionsWrapper);
    sectionsWidget->setMaximumWidth(1500);
    QHBoxLayout *sectionsLayout = new QHBoxLayout(sectionsWidget);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->setSpacing(12);

    auto createActionButton = [](const QString &text)
    {
        QPushButton *button = new QPushButton(text);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setMinimumHeight(36);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return button;
    };

    QGroupBox *databaseGroup = new QGroupBox("Zapisane maszyny", sectionsWidget);
    databaseGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *databaseLayout = new QVBoxLayout(databaseGroup);
    databaseLayout->setSpacing(10);

    QLabel *databaseNote = new QLabel("Maszyny zapisane w lokalnej bazie danych.", databaseGroup);
    databaseNote->setWordWrap(true);
    databaseLayout->addWidget(databaseNote);

    QScrollArea *databaseScroll = new QScrollArea(databaseGroup);
    databaseScroll->setObjectName("startMachineScroll");
    databaseScroll->setWidgetResizable(true);
    databaseScroll->setFrameShape(QFrame::NoFrame);
    databaseScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    databaseScroll->setMinimumHeight(260);

    QWidget *databaseButtons = new QWidget(databaseScroll);
    QVBoxLayout *databaseButtonsLayout = new QVBoxLayout(databaseButtons);
    databaseButtonsLayout->setContentsMargins(0, 0, 0, 0);
    databaseButtonsLayout->setSpacing(8);

    QStringList machineNames;
    if (QFile::exists("machines.db"))
    {
        machineNames = getMachineNamesFromDatabase();
    }
    machineNames.sort(Qt::CaseInsensitive);

    if (machineNames.isEmpty())
    {
        QLabel *emptyNote = new QLabel("Brak zapisanych maszyn. Po zapisie do bazy pojawią się w tym miejscu.", databaseButtons);
        emptyNote->setWordWrap(true);
        databaseButtonsLayout->addWidget(emptyNote);
    }
    else
    {
        const QFontMetrics metrics(page->font());
        for (const QString &machineName : machineNames)
        {
            const QString displayName = metrics.elidedText(machineName, Qt::ElideRight, 320);
            QPushButton *machineButton = createActionButton(displayName);
            machineButton->setToolTip(machineName);
            connect(machineButton, &QPushButton::clicked, this, [this, machineName]()
                    { openMachineFromDatabaseName(machineName); });
            databaseButtonsLayout->addWidget(machineButton);
        }
    }

    databaseButtonsLayout->addStretch(1);
    databaseScroll->setWidget(databaseButtons);
    databaseLayout->addWidget(databaseScroll);

    QGroupBox *projectGroup = new QGroupBox("Nowy projekt", sectionsWidget);
    projectGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *projectLayout = new QVBoxLayout(projectGroup);
    projectLayout->setSpacing(10);

    QLabel *projectNote = new QLabel("Wybierz gotowy punkt startowy.", projectGroup);
    projectNote->setWordWrap(true);
    projectLayout->addWidget(projectNote);

    QPushButton *blankButton = createActionButton("Pusty projekt");
    blankButton->setToolTip("Czysta konfiguracja maszyny do samodzielnego uzupełnienia.");
    connect(blankButton, &QPushButton::clicked, this, &TuringSimulatorUI::createNewMachineTab);
    projectLayout->addWidget(blankButton);

    QPushButton *exampleButton = createActionButton("Przykład");
    exampleButton->setToolTip("Gotowa maszyna startowa do szybkiego testu symulacji.");
    connect(exampleButton, &QPushButton::clicked, this, &TuringSimulatorUI::createExampleMachineTab);
    projectLayout->addWidget(exampleButton);

    QPushButton *aiButton = createActionButton("Start z AI");
    aiButton->setToolTip("Niedostępne w tej wersji.");
    aiButton->setEnabled(false);
    aiButton->setCursor(Qt::ArrowCursor);
    projectLayout->addWidget(aiButton);
    projectLayout->addStretch(1);

    sectionsLayout->addWidget(databaseGroup, 1);
    sectionsLayout->addWidget(projectGroup, 1);
    sectionsWrapperLayout->addStretch(1);
    sectionsWrapperLayout->addWidget(sectionsWidget, 1);
    sectionsWrapperLayout->addStretch(1);
    pageLayout->addWidget(sectionsWrapper, 1);
    pageLayout->addStretch(1);

    pageScrollArea->setWidget(pageWidget);
    mainLayout->addWidget(pageScrollArea);
    return page;
}

bool TuringSimulatorUI::isStartPage(QWidget *widget) const
{
    return widget && widget->objectName() == "startPage";
}

void TuringSimulatorUI::showStartPage()
{
    if (!machineTabs)
    {
        return;
    }

    for (int index = 0; index < machineTabs->count(); ++index)
    {
        if (isStartPage(machineTabs->widget(index)))
        {
            machineTabs->setCurrentIndex(index);
            updateMenuState();
            updateNewTabButtonPosition();
            return;
        }
    }

    addClosableTab(createStartPage(), "Start");
}

int TuringSimulatorUI::addClosableTab(QWidget *tab, const QString &title, int insertIndex)
{
    const int boundedIndex = insertIndex < 0
                                 ? machineTabs->count()
                                 : std::clamp(insertIndex, 0, machineTabs->count());
    const int index = machineTabs->insertTab(boundedIndex, tab, title);
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
            closeTabAt(tabIndex);
        } });

    updateMenuState();
    updateNewTabButtonPosition();
    return index;
}

void TuringSimulatorUI::addMachineTab(MachineEditorTab *tab, int insertIndex)
{
    addClosableTab(tab, tab->tabTitle(), insertIndex);

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

void TuringSimulatorUI::addMachineTabFromStartPage(MachineEditorTab *tab)
{
    const int currentIndex = machineTabs ? machineTabs->currentIndex() : -1;
    QWidget *currentTab = machineTabs ? machineTabs->currentWidget() : nullptr;

    if (isStartPage(currentTab))
    {
        machineTabs->removeTab(currentIndex);
        currentTab->deleteLater();
        addMachineTab(tab, currentIndex);
        return;
    }

    addMachineTab(tab);
}

void TuringSimulatorUI::createNewMachineTab()
{
    MachineEditorTab *tab = new MachineEditorTab(QString("Maszyna %1").arg(untitledTabCounter++), false, machineTabs);
    addMachineTabFromStartPage(tab);
}

void TuringSimulatorUI::createExampleMachineTab()
{
    MachineEditorTab *tab = new MachineEditorTab(QString("Maszyna %1").arg(untitledTabCounter++), true, machineTabs);
    addMachineTabFromStartPage(tab);
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

    addMachineTabFromStartPage(tab);
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
    if (!machineTabs)
    {
        return;
    }

    closeTabAt(machineTabs->currentIndex());
}

void TuringSimulatorUI::closeTabAt(int index)
{
    if (!machineTabs || index < 0 || index >= machineTabs->count())
    {
        return;
    }

    QWidget *currentTab = machineTabs->widget(index);
    machineTabs->removeTab(index);
    currentTab->deleteLater();

    if (machineTabs->count() == 0)
    {
        showStartPage();
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

void TuringSimulatorUI::toggleTheme()
{
    if (themeManager)
    {
        themeManager->toggleTheme();

        // Update menu text based on new theme
        if (toggleThemeAction)
        {
            QString newText = (themeManager->currentTheme() == ThemeManager::Theme::Dark)
                ? "Tryb jasny"
                : "Tryb ciemny";
            toggleThemeAction->setText(newText);
        }
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
    const bool hasCurrentTab = machineTabs && machineTabs->currentWidget() != nullptr;
    const bool hasMultipleTabs = machineTabs && machineTabs->count() > 1;

    saveAction->setEnabled(hasTab);
    saveAsAction->setEnabled(hasTab);
    closeTabAction->setEnabled(hasCurrentTab);
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


void TuringSimulatorUI::showAboutAuthorsDialog()
{
    QMessageBox::about(
        this,
        "O autorach",
        "Program został stworzony przez studentów Politechniki Śląskiej z wydziału matematyki stosowanej\n\nRadosław Kadela, Michał Talaga, Jonatan Turski");
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
        openMachineFromDatabaseName(chosenName);
    }
}

void TuringSimulatorUI::openMachineFromDatabaseName(const QString &machineName)
{
    MachineEditorTab *tab = new MachineEditorTab(machineName, false, machineTabs);

    if (!tab->loadFromDatabase(machineName)) {
        tab->deleteLater();
        return;
    }

    addMachineTabFromStartPage(tab);
}

void TuringSimulatorUI::saveCurrentMachineToDatabase()
{
    if (MachineEditorTab *tab = currentMachineTab()) {
        tab->saveToDatabase();
    }
}
