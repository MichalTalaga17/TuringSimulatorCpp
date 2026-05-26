#include "TuringSimulatorUI.h"

#include "ui/GeminiApiKeysDialog.h"
#include "ui/GeminiGenerationRunner.h"
#include "LanguageManager.h"

#include <QApplication>
#include <QColor>
#include <QDebug>
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
#include <QSettings>
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
      resetExampleAction(nullptr),
      generateAiAction(nullptr),
      geminiSettingsAction(nullptr),
      stepAction(nullptr),
      runAction(nullptr),
      resetAction(nullptr),
      previousTabAction(nullptr),
      nextTabAction(nullptr),
      toggleThemeAction(nullptr),
      untitledTabCounter(1),
      themeManager(std::make_unique<ThemeManager>())
{
    setWindowTitle(TR("main_window_title"));
    setGeometry(100, 100, 1200, 850);
    setMinimumWidth(980);
    setMinimumHeight(720);

    setupMenu();
    setupUI();
    showStartPage();
    updateMenuState();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, &TuringSimulatorUI::retranslateUi);
}

void TuringSimulatorUI::setupUI()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    // Motyw został już zainicjalizowany w konstruktorze przez themeManager.
    // Nie trzeba ponownie wywoływać setTheme, bo motyw jest już zastosowany.

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
    newTabButton->setToolTip(TR("new_machine_tooltip"));
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

    fileMenu = menuBar()->addMenu(TR("menu_file"));
    newTabAction = fileMenu->addAction(TR("menu_file_new"));
    bindShortcut(newTabAction, QKeySequence(QKeySequence::New));
    connect(newTabAction, &QAction::triggered, this, &TuringSimulatorUI::createNewMachineTab);

    exampleTabAction = fileMenu->addAction(TR("menu_file_example"));
    connect(exampleTabAction, &QAction::triggered, this, &TuringSimulatorUI::createExampleMachineTab);

    openAction = fileMenu->addAction(TR("menu_file_open"));
    bindShortcut(openAction, QKeySequence(QKeySequence::Open));
    connect(openAction, &QAction::triggered, this, &TuringSimulatorUI::openMachineInNewTab);

    fileMenu->addSeparator();

    saveAction = fileMenu->addAction(TR("menu_file_save"));
    bindShortcut(saveAction, QKeySequence(QKeySequence::Save));
    connect(saveAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachine);

    saveAsAction = fileMenu->addAction(TR("menu_file_save_as"));
    bindShortcut(saveAsAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachineAs);

    fileMenu->addSeparator();

    openDbAction = fileMenu->addAction(TR("menu_file_open_db"));
    connect(openDbAction, &QAction::triggered, this, &TuringSimulatorUI::openMachineFromDatabase);


    saveDbAction = fileMenu->addAction(TR("menu_file_save_db"));
    connect(saveDbAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachineToDatabase);

    fileMenu->addSeparator();

    closeTabAction = fileMenu->addAction(TR("menu_file_close_tab"));
    bindShortcut(closeTabAction, QKeySequence(QKeySequence::Close));
    connect(closeTabAction, &QAction::triggered, this, &TuringSimulatorUI::closeCurrentMachineTab);

    fileMenu->addSeparator();

    quitAction = fileMenu->addAction(TR("menu_file_quit"));
    bindShortcut(quitAction, QKeySequence(QKeySequence::Quit));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    machineMenu = menuBar()->addMenu(TR("menu_machine"));

    resetExampleAction = machineMenu->addAction(TR("menu_machine_reset_example"));
    connect(resetExampleAction, &QAction::triggered, this, &TuringSimulatorUI::resetCurrentMachineToExample);

    generateAiAction = machineMenu->addAction(TR("menu_machine_generate_ai"));
    bindShortcut(generateAiAction, QKeySequence(Qt::CTRL | Qt::Key_G));
    connect(generateAiAction, &QAction::triggered, this, &TuringSimulatorUI::generateMachineWithAi);

    geminiSettingsAction = machineMenu->addAction(TR("menu_machine_gemini_settings"));
    connect(geminiSettingsAction, &QAction::triggered, this, &TuringSimulatorUI::showGeminiSettingsDialog);

    simulationMenu = menuBar()->addMenu(TR("menu_simulation"));
    stepAction = simulationMenu->addAction(TR("menu_simulation_step"));
    bindShortcut(stepAction, QKeySequence(Qt::Key_F10));
    connect(stepAction, &QAction::triggered, this, &TuringSimulatorUI::stepCurrentMachine);

    runAction = simulationMenu->addAction(TR("menu_simulation_run_pause"));
    bindShortcut(runAction, QKeySequence(Qt::Key_F5));
    connect(runAction, &QAction::triggered, this, &TuringSimulatorUI::toggleCurrentMachineRun);

    resetAction = simulationMenu->addAction(TR("menu_simulation_reset"));
    bindShortcut(resetAction, QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(resetAction, &QAction::triggered, this, &TuringSimulatorUI::resetCurrentMachine);

    viewMenu = menuBar()->addMenu(TR("menu_view"));
    previousTabAction = viewMenu->addAction(TR("menu_view_prev_tab"));
    bindShortcut(previousTabAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(previousTabAction, &QAction::triggered, this, &TuringSimulatorUI::selectPreviousTab);

    nextTabAction = viewMenu->addAction(TR("menu_view_next_tab"));
    bindShortcut(nextTabAction, QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(nextTabAction, &QAction::triggered, this, &TuringSimulatorUI::selectNextTab);

    viewMenu->addSeparator();

    toggleThemeAction = viewMenu->addAction(TR("menu_view_dark_mode"));
    bindShortcut(toggleThemeAction, QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(toggleThemeAction, &QAction::triggered, this, &TuringSimulatorUI::toggleTheme);

    helpMenu = menuBar()->addMenu(TR("menu_help"));
    aboutAction = helpMenu->addAction(TR("menu_help_about"));
    connect(aboutAction, &QAction::triggered, this, &TuringSimulatorUI::showAboutDialog);

    aboutAuthorsAction = helpMenu->addAction(TR("menu_help_authors"));
    connect(aboutAuthorsAction, &QAction::triggered, this, &TuringSimulatorUI::showAboutAuthorsDialog);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    menuBar()->setCornerWidget(spacer, Qt::TopRightCorner);

    langButton = new QPushButton(menuBar());
    langButton->setFlat(true);
    langButton->setCursor(Qt::PointingHandCursor);
    QString currentLangIcon = LanguageManager::instance().currentLanguage() == LanguageManager::Language::Polish ? "🇵🇱" : "🇬🇧";
    langButton->setText(currentLangIcon);
    connect(langButton, &QPushButton::clicked, this, &TuringSimulatorUI::toggleLanguage);

    // Add the button to the right of the menubar native corner widget.
    menuBar()->setCornerWidget(langButton, Qt::TopRightCorner);
}

void TuringSimulatorUI::retranslateUi()
{
    setWindowTitle(TR("main_window_title"));
    if (newTabButton) {
        newTabButton->setToolTip(TR("new_machine_tooltip"));
    }

    if (fileMenu) fileMenu->setTitle(TR("menu_file"));
    if (newTabAction) newTabAction->setText(TR("menu_file_new"));
    if (exampleTabAction) exampleTabAction->setText(TR("menu_file_example"));
    if (openAction) openAction->setText(TR("menu_file_open"));
    if (openDbAction) openDbAction->setText(TR("menu_file_open_db"));
    if (saveDbAction) saveDbAction->setText(TR("menu_file_save_db"));
    if (saveAction) saveAction->setText(TR("menu_file_save"));
    if (saveAsAction) saveAsAction->setText(TR("menu_file_save_as"));
    if (closeTabAction) closeTabAction->setText(TR("menu_file_close_tab"));
    if (quitAction) quitAction->setText(TR("menu_file_quit"));

    if (machineMenu) machineMenu->setTitle(TR("menu_machine"));
    if (resetExampleAction) resetExampleAction->setText(TR("menu_machine_reset_example"));
    if (generateAiAction) generateAiAction->setText(TR("menu_machine_generate_ai"));
    if (geminiSettingsAction) geminiSettingsAction->setText(TR("menu_machine_gemini_settings"));

    if (simulationMenu) simulationMenu->setTitle(TR("menu_simulation"));
    if (stepAction) stepAction->setText(TR("menu_simulation_step"));
    if (runAction) runAction->setText(TR("menu_simulation_run_pause"));
    if (resetAction) resetAction->setText(TR("menu_simulation_reset"));

    if (viewMenu) viewMenu->setTitle(TR("menu_view"));
    if (previousTabAction) previousTabAction->setText(TR("menu_view_prev_tab"));
    if (nextTabAction) nextTabAction->setText(TR("menu_view_next_tab"));
    if (toggleThemeAction) toggleThemeAction->setText(TR("menu_view_dark_mode"));

    if (helpMenu) helpMenu->setTitle(TR("menu_help"));
    if (aboutAction) aboutAction->setText(TR("menu_help_about"));
    if (aboutAuthorsAction) aboutAuthorsAction->setText(TR("menu_help_authors"));

    if (langButton) {
        QString currentLangIcon = LanguageManager::instance().currentLanguage() == LanguageManager::Language::Polish ? "🇵🇱" : "🇬🇧";
        langButton->setText(currentLangIcon);
    }
}

void TuringSimulatorUI::toggleLanguage()
{
    auto& lm = LanguageManager::instance();
    auto newLang = (lm.currentLanguage() == LanguageManager::Language::Polish) 
                   ? LanguageManager::Language::English 
                   : LanguageManager::Language::Polish;

    lm.setLanguage(newLang);
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
    aiButton->setToolTip("Opisz maszynę, a Gemini AI wypełni edytor.");
    connect(aiButton, &QPushButton::clicked, this, &TuringSimulatorUI::generateMachineWithAi);
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

void TuringSimulatorUI::resetCurrentMachineToExample()
{
    if (MachineEditorTab *tab = currentMachineTab())
    {
        tab->resetToDefaultExample();
    }
}

void TuringSimulatorUI::generateMachineWithAi()
{
    MachineEditorTab *tab = currentMachineTab();
    if (tab)
    {
        tab->generateMachineWithAi();
        return;
    }

    qInfo().noquote() << "[GeminiAI] No machine tab active; asking before creating a new tab";

    QString description;
    QString apiKey;
    QString modelName;
    QString fallbackModelName;
    bool thinkingEnabled = false;
    if (!MachineEditorTab::resolveGeminiGenerationRequest(this, &description, &apiKey, &modelName, &fallbackModelName, &thinkingEnabled))
    {
        qInfo().noquote() << "[GeminiAI] Canceled before creating a new machine";
        return;
    }

    qInfo().noquote() << "[GeminiAI] Requesting machine before creating a new tab";

    GeminiGenerationRequest request{
        description.trimmed(),
        apiKey,
        modelName,
        fallbackModelName,
        thinkingEnabled};

    runGeminiGenerationWithProgress(
        this,
        request,
        [this](const GeminiMachineGenerator::Result &result)
        {
            if (!result.success)
            {
                qWarning().noquote() << "[GeminiAI] New-tab generation failed:"
                                     << result.errorMessage;
                QMessageBox::warning(this, "Gemini AI", result.errorMessage);
                return;
            }

            qInfo().noquote() << "[GeminiAI] Creating machine tab after Gemini response";
            MachineEditorTab *generatedTab = new MachineEditorTab(QString("Maszyna %1").arg(untitledTabCounter++), false, machineTabs);
            addMachineTabFromStartPage(generatedTab);
            generatedTab->applyGeneratedMachineData(result.machineData);
            QMessageBox::information(this, "Gemini AI", "Maszyna została wygenerowana i wczytana do edytora.");
        },
        []()
        {
            qInfo().noquote() << "[GeminiAI] User canceled request before tab creation";
        });
}

void TuringSimulatorUI::showGeminiSettingsDialog()
{
    QSettings settings;
    GeminiApiKeysDialog::editStoredKeys(this, settings);
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

        // Zaktualizuj tekst menu na podstawie nowego motywu.
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
    resetExampleAction->setEnabled(hasTab);
    generateAiAction->setEnabled(true);
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
