#include "TuringSimulatorUI.h"

#include "LanguageManager.h"
#include "ui/GeminiApiKeysDialog.h"
#include "ui/GeminiGenerationRunner.h"

#include <QActionGroup>
#include <QApplication>
#include <QDebug>
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
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QStyleFactory>
#include <QTabBar>
#include <QVBoxLayout>
#include <algorithm>
#include <vector>

TuringSimulatorUI::TuringSimulatorUI(QWidget *parent)
    : QMainWindow(parent),
      machineTabs(nullptr),
      newTabButton(nullptr),
      fileMenu(nullptr),
      machineMenu(nullptr),
      simulationMenu(nullptr),
      viewMenu(nullptr),
      languageMenu(nullptr),
      helpMenu(nullptr),
      newTabAction(nullptr),
      exampleTabAction(nullptr),
      openAction(nullptr),
      openDbAction(nullptr),
      saveDbAction(nullptr),
      quitAction(nullptr),
      aboutAction(nullptr),
      aboutAuthorsAction(nullptr),
      polishLanguageAction(nullptr),
      englishLanguageAction(nullptr),
      languageActionGroup(nullptr),
      saveAction(nullptr),
      saveAsAction(nullptr),
      closeTabAction(nullptr),
      validateAction(nullptr),
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
    setWindowTitle(LanguageManager::text("app_title"));
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
    newTabButton->setToolTip(LanguageManager::text("new_tab_tooltip"));
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

    fileMenu = menuBar()->addMenu(LanguageManager::text("menu_file"));
    newTabAction = fileMenu->addAction(LanguageManager::text("action_new_blank"));
    bindShortcut(newTabAction, QKeySequence(QKeySequence::New));
    connect(newTabAction, &QAction::triggered, this, &TuringSimulatorUI::createNewMachineTab);

    exampleTabAction = fileMenu->addAction(LanguageManager::text("action_new_example"));
    connect(exampleTabAction, &QAction::triggered, this, &TuringSimulatorUI::createExampleMachineTab);

    openAction = fileMenu->addAction(LanguageManager::text("action_open_json"));
    bindShortcut(openAction, QKeySequence(QKeySequence::Open));
    connect(openAction, &QAction::triggered, this, &TuringSimulatorUI::openMachineInNewTab);

    fileMenu->addSeparator();

    saveAction = fileMenu->addAction(LanguageManager::text("action_save"));
    bindShortcut(saveAction, QKeySequence(QKeySequence::Save));
    connect(saveAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachine);

    saveAsAction = fileMenu->addAction(LanguageManager::text("action_save_as"));
    bindShortcut(saveAsAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachineAs);

    fileMenu->addSeparator();

    openDbAction = fileMenu->addAction(LanguageManager::text("action_open_db"));
    connect(openDbAction, &QAction::triggered, this, &TuringSimulatorUI::openMachineFromDatabase);


    saveDbAction = fileMenu->addAction(LanguageManager::text("action_save_db"));
    connect(saveDbAction, &QAction::triggered, this, &TuringSimulatorUI::saveCurrentMachineToDatabase);

    fileMenu->addSeparator();

    closeTabAction = fileMenu->addAction(LanguageManager::text("action_close_tab"));
    bindShortcut(closeTabAction, QKeySequence(QKeySequence::Close));
    connect(closeTabAction, &QAction::triggered, this, &TuringSimulatorUI::closeCurrentMachineTab);

    fileMenu->addSeparator();

    quitAction = fileMenu->addAction(LanguageManager::text("action_quit"));
    bindShortcut(quitAction, QKeySequence(QKeySequence::Quit));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    machineMenu = menuBar()->addMenu(LanguageManager::text("menu_machine"));
    validateAction = machineMenu->addAction(LanguageManager::text("action_validate"));
    bindShortcut(validateAction, QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(validateAction, &QAction::triggered, this, &TuringSimulatorUI::validateCurrentMachine);

    resetExampleAction = machineMenu->addAction(LanguageManager::text("action_reset_example"));
    connect(resetExampleAction, &QAction::triggered, this, &TuringSimulatorUI::resetCurrentMachineToExample);

    generateAiAction = machineMenu->addAction(LanguageManager::text("action_generate_ai"));
    bindShortcut(generateAiAction, QKeySequence(Qt::CTRL | Qt::Key_G));
    connect(generateAiAction, &QAction::triggered, this, &TuringSimulatorUI::generateMachineWithAi);

    geminiSettingsAction = machineMenu->addAction(LanguageManager::text("action_gemini_settings"));
    connect(geminiSettingsAction, &QAction::triggered, this, &TuringSimulatorUI::showGeminiSettingsDialog);

    simulationMenu = menuBar()->addMenu(LanguageManager::text("menu_simulation"));
    stepAction = simulationMenu->addAction(LanguageManager::text("action_step"));
    bindShortcut(stepAction, QKeySequence(Qt::Key_F10));
    connect(stepAction, &QAction::triggered, this, &TuringSimulatorUI::stepCurrentMachine);

    runAction = simulationMenu->addAction(LanguageManager::text("action_run_pause"));
    bindShortcut(runAction, QKeySequence(Qt::Key_F5));
    connect(runAction, &QAction::triggered, this, &TuringSimulatorUI::toggleCurrentMachineRun);

    resetAction = simulationMenu->addAction(LanguageManager::text("action_reset"));
    bindShortcut(resetAction, QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(resetAction, &QAction::triggered, this, &TuringSimulatorUI::resetCurrentMachine);

    viewMenu = menuBar()->addMenu(LanguageManager::text("menu_view"));
    previousTabAction = viewMenu->addAction(LanguageManager::text("action_previous_tab"));
    bindShortcut(previousTabAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(previousTabAction, &QAction::triggered, this, &TuringSimulatorUI::selectPreviousTab);

    nextTabAction = viewMenu->addAction(LanguageManager::text("action_next_tab"));
    bindShortcut(nextTabAction, QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(nextTabAction, &QAction::triggered, this, &TuringSimulatorUI::selectNextTab);

    viewMenu->addSeparator();

    toggleThemeAction = viewMenu->addAction(LanguageManager::text("action_dark_mode"));
    bindShortcut(toggleThemeAction, QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(toggleThemeAction, &QAction::triggered, this, &TuringSimulatorUI::toggleTheme);

    viewMenu->addSeparator();
    languageMenu = viewMenu->addMenu(LanguageManager::text("menu_language"));
    languageActionGroup = new QActionGroup(this);
    languageActionGroup->setExclusive(true);
    polishLanguageAction = languageMenu->addAction(LanguageManager::text("language_polish"));
    englishLanguageAction = languageMenu->addAction(LanguageManager::text("language_english"));
    polishLanguageAction->setCheckable(true);
    englishLanguageAction->setCheckable(true);
    languageActionGroup->addAction(polishLanguageAction);
    languageActionGroup->addAction(englishLanguageAction);
    polishLanguageAction->setChecked(LanguageManager::currentLanguage() == LanguageManager::Language::Polish);
    englishLanguageAction->setChecked(LanguageManager::currentLanguage() == LanguageManager::Language::English);
    connect(polishLanguageAction, &QAction::triggered, this, &TuringSimulatorUI::setPolishLanguage);
    connect(englishLanguageAction, &QAction::triggered, this, &TuringSimulatorUI::setEnglishLanguage);

    helpMenu = menuBar()->addMenu(LanguageManager::text("menu_help"));
    aboutAction = helpMenu->addAction(LanguageManager::text("action_about_app"));
    connect(aboutAction, &QAction::triggered, this, &TuringSimulatorUI::showAboutDialog);

    aboutAuthorsAction = helpMenu->addAction(LanguageManager::text("action_about_authors"));
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

    QGroupBox *databaseGroup = new QGroupBox(LanguageManager::text("start_saved_machines"), sectionsWidget);
    databaseGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *databaseLayout = new QVBoxLayout(databaseGroup);
    databaseLayout->setSpacing(10);

    QLabel *databaseNote = new QLabel(LanguageManager::text("start_saved_note"), databaseGroup);
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

    QStringList machineNames = getMachineNamesFromDatabase();
    machineNames.sort(Qt::CaseInsensitive);

    if (machineNames.isEmpty())
    {
        QLabel *emptyNote = new QLabel(LanguageManager::text("start_empty_saved"), databaseButtons);
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

    QGroupBox *projectGroup = new QGroupBox(LanguageManager::text("start_new_project"), sectionsWidget);
    projectGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *projectLayout = new QVBoxLayout(projectGroup);
    projectLayout->setSpacing(10);

    QLabel *projectNote = new QLabel(LanguageManager::text("start_project_note"), projectGroup);
    projectNote->setWordWrap(true);
    projectLayout->addWidget(projectNote);

    QPushButton *blankButton = createActionButton(LanguageManager::text("start_blank_project"));
    blankButton->setToolTip(LanguageManager::text("start_blank_tooltip"));
    connect(blankButton, &QPushButton::clicked, this, &TuringSimulatorUI::createNewMachineTab);
    projectLayout->addWidget(blankButton);

    QPushButton *exampleButton = createActionButton(LanguageManager::text("start_example"));
    exampleButton->setToolTip(LanguageManager::text("start_example_tooltip"));
    connect(exampleButton, &QPushButton::clicked, this, &TuringSimulatorUI::createExampleMachineTab);
    projectLayout->addWidget(exampleButton);

    QPushButton *aiButton = createActionButton(LanguageManager::text("start_ai"));
    aiButton->setToolTip(LanguageManager::text("start_ai_tooltip"));
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

    addClosableTab(createStartPage(), LanguageManager::text("tab_start"));
}

int TuringSimulatorUI::addClosableTab(QWidget *tab, const QString &title, int insertIndex)
{
    const int boundedIndex = insertIndex < 0
                                 ? machineTabs->count()
                                 : std::clamp(insertIndex, 0, machineTabs->count());
    const int index = machineTabs->insertTab(boundedIndex, tab, title);
    machineTabs->setCurrentIndex(index);
    updateNewTabButtonPosition();

    installCloseButtonForTab(tab, index);

    updateMenuState();
    updateNewTabButtonPosition();
    return index;
}

void TuringSimulatorUI::installCloseButtonForTab(QWidget *tab, int index)
{
    QWidget *closeButtonWrapper = new QWidget(machineTabs);
    QHBoxLayout *closeButtonLayout = new QHBoxLayout(closeButtonWrapper);
    closeButtonLayout->setContentsMargins(0, 0, 6, 0);
    closeButtonLayout->setSpacing(0);

    QPushButton *closeButton = new QPushButton("x", closeButtonWrapper);
    closeButton->setObjectName("tabCloseButton");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFocusPolicy(Qt::NoFocus);
    closeButton->setToolTip(LanguageManager::text("close_tab_tooltip"));
    closeButtonLayout->addWidget(closeButton);
    machineTabs->tabBar()->setTabButton(index, QTabBar::RightSide, closeButtonWrapper);

    connect(closeButton, &QPushButton::clicked, this, [this, tab]()
            {
        const int tabIndex = machineTabs->indexOf(tab);
        if (tabIndex >= 0)
        {
            closeTabAt(tabIndex);
        } });
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
    MachineEditorTab *tab = new MachineEditorTab(LanguageManager::text("untitled_machine").arg(untitledTabCounter++), false, machineTabs);
    addMachineTabFromStartPage(tab);
}

void TuringSimulatorUI::createExampleMachineTab()
{
    MachineEditorTab *tab = new MachineEditorTab(LanguageManager::text("untitled_machine").arg(untitledTabCounter++), true, machineTabs);
    addMachineTabFromStartPage(tab);
}

void TuringSimulatorUI::openMachineInNewTab()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        LanguageManager::text("dialog_open_machine"),
        "",
        LanguageManager::text("file_filter_json"));
    if (filePath.isEmpty())
    {
        return;
    }

    MachineEditorTab *tab = new MachineEditorTab(LanguageManager::text("untitled_machine").arg(untitledTabCounter++), false, machineTabs);
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

void TuringSimulatorUI::generateMachineWithAi()
{
    MachineEditorTab *tab = currentMachineTab();
    if (tab)
    {
        tab->generateMachineWithAi();
        return;
    }

    qInfo().noquote() << "[GeminiAI] Brak aktywnej karty maszyny; pytam przed utworzeniem nowej karty";

    QString description;
    QString apiKey;
    QString modelName;
    QString fallbackModelName;
    bool thinkingEnabled = false;
    if (!MachineEditorTab::resolveGeminiGenerationRequest(this, &description, &apiKey, &modelName, &fallbackModelName, &thinkingEnabled))
    {
        qInfo().noquote() << "[GeminiAI] Anulowano przed utworzeniem nowej maszyny";
        return;
    }

    qInfo().noquote() << "[GeminiAI] Pobieram maszynę przed utworzeniem nowej karty";

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
                qWarning().noquote() << "[GeminiAI] Generowanie dla nowej karty nie powiodło się:"
                                     << result.errorMessage;
                QMessageBox::warning(this, LanguageManager::text("gemini_title"), result.errorMessage);
                return;
            }

            qInfo().noquote() << "[GeminiAI] Tworzę kartę maszyny po odpowiedzi Gemini";
            MachineEditorTab *generatedTab = new MachineEditorTab(LanguageManager::text("untitled_machine").arg(untitledTabCounter++), false, machineTabs);
            addMachineTabFromStartPage(generatedTab);
            generatedTab->applyGeneratedMachineData(result.machineData);
            QMessageBox::information(this, LanguageManager::text("gemini_title"), LanguageManager::text("gemini_generated_success"));
        },
        []()
        {
            qInfo().noquote() << "[GeminiAI] Użytkownik anulował żądanie przed utworzeniem karty";
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
        retranslateUi();
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

void TuringSimulatorUI::setPolishLanguage()
{
    setLanguage(LanguageManager::Language::Polish);
}

void TuringSimulatorUI::setEnglishLanguage()
{
    setLanguage(LanguageManager::Language::English);
}

void TuringSimulatorUI::setLanguage(LanguageManager::Language language)
{
    LanguageManager::setCurrentLanguage(language);

    if (polishLanguageAction)
    {
        polishLanguageAction->setChecked(language == LanguageManager::Language::Polish);
    }
    if (englishLanguageAction)
    {
        englishLanguageAction->setChecked(language == LanguageManager::Language::English);
    }

    if (machineTabs)
    {
        for (int index = 0; index < machineTabs->count(); ++index)
        {
            if (MachineEditorTab *tab = qobject_cast<MachineEditorTab *>(machineTabs->widget(index)))
            {
                tab->retranslateUi();
                machineTabs->setTabText(index, tab->tabTitle());
            }
        }
    }

    retranslateUi();
    refreshStartPages();
    updateMenuState();
}

void TuringSimulatorUI::retranslateUi()
{
    if (fileMenu) fileMenu->setTitle(LanguageManager::text("menu_file"));
    if (machineMenu) machineMenu->setTitle(LanguageManager::text("menu_machine"));
    if (simulationMenu) simulationMenu->setTitle(LanguageManager::text("menu_simulation"));
    if (viewMenu) viewMenu->setTitle(LanguageManager::text("menu_view"));
    if (languageMenu) languageMenu->setTitle(LanguageManager::text("menu_language"));
    if (helpMenu) helpMenu->setTitle(LanguageManager::text("menu_help"));

    if (newTabAction) newTabAction->setText(LanguageManager::text("action_new_blank"));
    if (exampleTabAction) exampleTabAction->setText(LanguageManager::text("action_new_example"));
    if (openAction) openAction->setText(LanguageManager::text("action_open_json"));
    if (saveAction) saveAction->setText(LanguageManager::text("action_save"));
    if (saveAsAction) saveAsAction->setText(LanguageManager::text("action_save_as"));
    if (openDbAction) openDbAction->setText(LanguageManager::text("action_open_db"));
    if (saveDbAction) saveDbAction->setText(LanguageManager::text("action_save_db"));
    if (closeTabAction) closeTabAction->setText(LanguageManager::text("action_close_tab"));
    if (quitAction) quitAction->setText(LanguageManager::text("action_quit"));
    if (validateAction) validateAction->setText(LanguageManager::text("action_validate"));
    if (resetExampleAction) resetExampleAction->setText(LanguageManager::text("action_reset_example"));
    if (generateAiAction) generateAiAction->setText(LanguageManager::text("action_generate_ai"));
    if (geminiSettingsAction) geminiSettingsAction->setText(LanguageManager::text("action_gemini_settings"));
    if (stepAction) stepAction->setText(LanguageManager::text("action_step"));
    if (runAction) runAction->setText(LanguageManager::text("action_run_pause"));
    if (resetAction) resetAction->setText(LanguageManager::text("action_reset"));
    if (previousTabAction) previousTabAction->setText(LanguageManager::text("action_previous_tab"));
    if (nextTabAction) nextTabAction->setText(LanguageManager::text("action_next_tab"));
    if (toggleThemeAction)
    {
        const bool darkTheme = themeManager && themeManager->currentTheme() == ThemeManager::Theme::Dark;
        toggleThemeAction->setText(LanguageManager::text(darkTheme ? "action_light_mode" : "action_dark_mode"));
    }
    if (polishLanguageAction) polishLanguageAction->setText(LanguageManager::text("language_polish"));
    if (englishLanguageAction) englishLanguageAction->setText(LanguageManager::text("language_english"));
    if (aboutAction) aboutAction->setText(LanguageManager::text("action_about_app"));
    if (aboutAuthorsAction) aboutAuthorsAction->setText(LanguageManager::text("action_about_authors"));

    if (newTabButton)
    {
        newTabButton->setToolTip(LanguageManager::text("new_tab_tooltip"));
    }

    if (machineTabs)
    {
        for (int index = 0; index < machineTabs->count(); ++index)
        {
            if (isStartPage(machineTabs->widget(index)))
            {
                machineTabs->setTabText(index, LanguageManager::text("tab_start"));
            }

            QWidget *closeButtonWrapper = machineTabs->tabBar()->tabButton(index, QTabBar::RightSide);
            if (closeButtonWrapper)
            {
                if (QPushButton *closeButton = closeButtonWrapper->findChild<QPushButton *>())
                {
                    closeButton->setToolTip(LanguageManager::text("close_tab_tooltip"));
                }
            }
        }
    }
}

void TuringSimulatorUI::refreshStartPages()
{
    if (!machineTabs)
    {
        return;
    }

    QWidget *currentWidgetBeforeRefresh = machineTabs->currentWidget();
    QWidget *replacementCurrentWidget = nullptr;
    std::vector<int> startPageIndexes;
    for (int index = 0; index < machineTabs->count(); ++index)
    {
        if (isStartPage(machineTabs->widget(index)))
        {
            startPageIndexes.push_back(index);
        }
    }

    for (auto iterator = startPageIndexes.rbegin(); iterator != startPageIndexes.rend(); ++iterator)
    {
        const int index = *iterator;
        QWidget *oldStartPage = machineTabs->widget(index);
        const bool wasCurrentWidget = oldStartPage == currentWidgetBeforeRefresh;

        machineTabs->removeTab(index);
        oldStartPage->deleteLater();

        QWidget *newStartPage = createStartPage();
        const int newIndex = machineTabs->insertTab(index, newStartPage, LanguageManager::text("tab_start"));
        installCloseButtonForTab(newStartPage, newIndex);
        if (wasCurrentWidget)
        {
            replacementCurrentWidget = newStartPage;
        }
    }

    if (replacementCurrentWidget)
    {
        machineTabs->setCurrentWidget(replacementCurrentWidget);
    }
    else if (currentWidgetBeforeRefresh && machineTabs->indexOf(currentWidgetBeforeRefresh) >= 0)
    {
        machineTabs->setCurrentWidget(currentWidgetBeforeRefresh);
    }
    else if (machineTabs->count() == 0)
    {
        showStartPage();
    }

    updateNewTabButtonPosition();
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

    QString title = LanguageManager::text("app_title");
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
        LanguageManager::text("action_about_app"),
        LanguageManager::text("about_text"));
}


void TuringSimulatorUI::showAboutAuthorsDialog()
{
    QMessageBox::about(
        this,
        LanguageManager::text("action_about_authors"),
        LanguageManager::text("authors_text"));
}
void TuringSimulatorUI::openMachineFromDatabase()
{
    // 1. Pobieramy listę dostępnych nazw maszyn z bazy
    QStringList names = getMachineNamesFromDatabase();
    if (names.isEmpty()) {
        QMessageBox::information(this, LanguageManager::text("database_title"), LanguageManager::text("database_empty_text"));
        return;
    }

    // 2. Wyświetlamy okno wyboru
    bool ok;
    QString chosenName = QInputDialog::getItem(
        this,
        LanguageManager::text("database_load_title"),
        LanguageManager::text("database_choose_machine"),
        names,
        0,
        false,
        &ok);

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
