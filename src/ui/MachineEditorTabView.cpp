#include "MachineEditorTab.h"
#include "LanguageManager.h"
#include "TransitionDelegate.h"

#include <QAbstractSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QListView>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

MachineEditorTab::MachineEditorTab(const QString &untitledTitle, bool startWithExample, QWidget *parent)
    : QWidget(parent),
      numStatesSpinBox(nullptr),
      alphabetInputField(nullptr),
      blankSymbolField(nullptr),
      initialStateCombo(nullptr),
      aiGenerateButton(nullptr),
      transitionsTable(nullptr),
      logicEditorGroup(nullptr),
      statesLabel(nullptr),
      alphabetLabel(nullptr),
      blankLabel(nullptr),
      initialLabel(nullptr),
      machineStateLabel(nullptr),
      stepCountLabel(nullptr),
      stepMetricTitleLabel(nullptr),
      stateMetricTitleLabel(nullptr),
      modeMetricTitleLabel(nullptr),
      tapeScrollArea(nullptr),
      tapeDisplay(nullptr),
      tapeInputField(nullptr),
      tapeWarningLabel(nullptr),
      stepButton(nullptr),
      runButton(nullptr),
      resetButton(nullptr),
      speedSlider(nullptr),
      speedLabel(nullptr),
      statusLabel(nullptr),
      tapeInputLabel(nullptr),
      statusGroup(nullptr),
      inputGroup(nullptr),
      tapeGroup(nullptr),
      machine(nullptr),
      blankSymbol('_'),
      isRunning(false),
      executionSpeed(500),
      executionTimer(std::make_unique<QTimer>()),
      tapeDisplayRefreshTimer(std::make_unique<QTimer>()),
      untitledTitle(untitledTitle),
      startWithExample(startWithExample),
      highlightedTransitionRow(-1),
      highlightedTransitionCol(-1),
      aiGenerationInProgress(false),
      currentStatusKey("status_ready")
{
    setupUI();

    // Podłącz timer wykonywania maszyny.
    connect(executionTimer.get(), &QTimer::timeout, this, &MachineEditorTab::onExecutionTimer);
    tapeDisplayRefreshTimer->setSingleShot(true);
    connect(tapeDisplayRefreshTimer.get(), &QTimer::timeout, this, &MachineEditorTab::updateTapeDisplay);

    scheduleTapeDisplayRefresh();
}

void MachineEditorTab::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *tabContentFade = new QFrame(this);
    tabContentFade->setObjectName("tabContentFade");
    tabContentFade->setFrameShape(QFrame::NoFrame);
    tabContentFade->setFixedHeight(8);
    mainLayout->addWidget(tabContentFade);

    QScrollArea *pageScrollArea = new QScrollArea(this);
    pageScrollArea->setWidgetResizable(true);
    pageScrollArea->setFrameShape(QFrame::NoFrame);
    pageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *pageWidget = new QWidget(pageScrollArea);
    QVBoxLayout *pageLayout = new QVBoxLayout(pageWidget);
    pageLayout->setContentsMargins(16, 4, 16, 16);
    pageLayout->setSpacing(12);

    // --- PANEL WYKONANIA NAJPIERW (żeby statusLabel powstał wcześnie) ---
    pageLayout->addWidget(setupExecutorTab(), 0);

    // --- PANEL BUDOWANIA POTEM (może już bezpiecznie używać statusLabel) ---
    pageLayout->addWidget(setupBuilderTab(), 1);

    pageScrollArea->setWidget(pageWidget);
    mainLayout->addWidget(pageScrollArea);
}

void MachineEditorTab::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!machine || !tapeScrollArea || tapeCells.empty())
    {
        return;
    }

    scheduleTapeDisplayRefresh();
}

QWidget *MachineEditorTab::setupBuilderTab()
{
    QWidget *builderWidget = new QWidget();
    builderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *builderLayout = new QVBoxLayout(builderWidget);
    builderLayout->setSpacing(12);
    builderLayout->setContentsMargins(0, 0, 0, 0);

    logicEditorGroup = new QGroupBox(LanguageManager::text("group_logic_editor"), builderWidget);
    logicEditorGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *logicEditorLayout = new QVBoxLayout(logicEditorGroup);
    logicEditorLayout->setSpacing(10);

    QHBoxLayout *configFieldsLayout = new QHBoxLayout();
    configFieldsLayout->setSpacing(10);

    statesLabel = new QLabel(LanguageManager::text("label_states"));
    configFieldsLayout->addWidget(statesLabel);
    numStatesSpinBox = new QSpinBox();
    numStatesSpinBox->setObjectName("numStatesSpinBox");
    numStatesSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    numStatesSpinBox->setMinimum(1);
    numStatesSpinBox->setMaximum(20);
    numStatesSpinBox->setValue(2);
    numStatesSpinBox->setFixedHeight(31);
    numStatesSpinBox->setMaximumWidth(72);
    configFieldsLayout->addWidget(numStatesSpinBox);

    alphabetLabel = new QLabel(LanguageManager::text("label_alphabet"));
    configFieldsLayout->addWidget(alphabetLabel);
    alphabetInputField = new QLineEdit();
    alphabetInputField->setPlaceholderText("0,1");
    // Ogranicz do standardowych drukowalnych znaków ASCII (32-126).
    alphabetInputField->setValidator(new QRegularExpressionValidator(QRegularExpression("[ -~]*"), this));
    alphabetInputField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    configFieldsLayout->addWidget(alphabetInputField, 1);

    blankLabel = new QLabel(LanguageManager::text("label_blank"));
    configFieldsLayout->addWidget(blankLabel);
    blankSymbolField = new QLineEdit();
    blankSymbolField->setText("_");
    // Ogranicz do jednego standardowego drukowalnego znaku ASCII bez spacji.
    blankSymbolField->setValidator(new QRegularExpressionValidator(QRegularExpression("[!-~]"), this));
    blankSymbolField->setMaximumWidth(60);
    configFieldsLayout->addWidget(blankSymbolField);

    initialLabel = new QLabel(LanguageManager::text("label_start"));
    configFieldsLayout->addWidget(initialLabel);
    initialStateCombo = new QComboBox();
    initialStateCombo->setObjectName("initialStateCombo");
    QListView *initialStatePopup = new QListView(initialStateCombo);
    initialStatePopup->setObjectName("initialStateComboPopup");
    initialStatePopup->setFrameShape(QFrame::NoFrame);
    initialStatePopup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    initialStatePopup->setAttribute(Qt::WA_TranslucentBackground);
    initialStatePopup->setAutoFillBackground(false);
    initialStatePopup->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    initialStatePopup->viewport()->setAutoFillBackground(false);
    initialStatePopup->setStyleSheet(R"(
        QListView#initialStateComboPopup {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            background-color: #ffffff;
            color: #202020;
            outline: 0;
            padding: 2px;
        }
        QListView#initialStateComboPopup::item {
            min-height: 24px;
            padding: 4px 8px;
            border-radius: 4px;
        }
        QListView#initialStateComboPopup::item:hover,
        QListView#initialStateComboPopup::item:selected {
            background-color: #eaf4ff;
            color: #005a9e;
        }
    )");
    initialStateCombo->setView(initialStatePopup);
    initialStateCombo->setMinimumWidth(90);
    initialStateCombo->setMaximumWidth(120);
    configFieldsLayout->addWidget(initialStateCombo);

    aiGenerateButton = new QPushButton("AI");
    aiGenerateButton->setToolTip(LanguageManager::text("ai_button_tooltip"));
    aiGenerateButton->setMinimumHeight(31);
    aiGenerateButton->setMinimumWidth(46);
    aiGenerateButton->setCursor(Qt::PointingHandCursor);
    configFieldsLayout->addWidget(aiGenerateButton);

    logicEditorLayout->addLayout(configFieldsLayout);

    transitionsTable = new QTableWidget();
    transitionsTable->setObjectName("transitionsTable");
    transitionsTable->setFrameShape(QFrame::NoFrame);
    transitionsTable->setShowGrid(false);
    transitionsTable->setAlternatingRowColors(true);
    transitionsTable->setWordWrap(false);
    transitionsTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    transitionsTable->setFocusPolicy(Qt::StrongFocus);
    transitionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    transitionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    transitionsTable->horizontalHeader()->setSectionsMovable(false);
    transitionsTable->horizontalHeader()->setDefaultSectionSize(132);
    transitionsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    transitionsTable->horizontalHeader()->setHighlightSections(false);
    transitionsTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    transitionsTable->verticalHeader()->setSectionsMovable(false);
    transitionsTable->verticalHeader()->setDefaultSectionSize(38);
    transitionsTable->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    transitionsTable->verticalHeader()->setHighlightSections(false);
    transitionsTable->setRowHeight(0, 45);
    transitionsTable->setItemDelegate(new TransitionDelegate(this, transitionsTable));
    logicEditorLayout->addWidget(transitionsTable, 1);

    builderLayout->addWidget(logicEditorGroup, 1);

    // Podłącz sygnały po utworzeniu wszystkich widżetów.
    connect(numStatesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MachineEditorTab::onNumStatesChanged);
    connect(alphabetInputField, &QLineEdit::textChanged, this, &MachineEditorTab::onAlphabetChanged);
    connect(blankSymbolField, &QLineEdit::textChanged, this, &MachineEditorTab::onBlankSymbolChanged);
    connect(aiGenerateButton, &QPushButton::clicked, this, [this]()
            { generateMachineWithAi(); });
    connect(initialStateCombo, &QComboBox::currentTextChanged, this, [this](const QString &state)
            {
        initialState = state.toStdString();
        if (rebuildMachineFromEditor()) {
            setStatusText("status_rebuilt");
        } });
    
    connect(transitionsTable, &QTableWidget::itemChanged, this, &MachineEditorTab::onTableItemChanged);

    // Zainicjalizuj przy bezpiecznie zablokowanych sygnałach.
    transitionsTable->blockSignals(true);
    onNumStatesChanged(2);
    transitionsTable->blockSignals(false);
    if (startWithExample)
    {
        applyDefaultExample();
    }
    else
    {
        applyBlankMachine();
    }

    return builderWidget;
}

QWidget *MachineEditorTab::setupExecutorTab()
{
    QWidget *executorWidget = new QWidget();
    executorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QVBoxLayout *executorLayout = new QVBoxLayout(executorWidget);
    executorLayout->setSpacing(12);

    // --- PANEL STATUSU ---
    statusGroup = new QGroupBox(LanguageManager::text("group_machine_state"), executorWidget);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);
    statusLayout->setSpacing(6);

    auto createStatusMetric = [this](const QString &title, QLabel *&titleLabel, QLabel *&valueLabel, const QString &value, int minWidth)
    {
        QWidget *metricWidget = new QWidget(statusGroup);
        metricWidget->setMinimumWidth(minWidth);

        QVBoxLayout *metricLayout = new QVBoxLayout(metricWidget);
        metricLayout->setContentsMargins(0, 0, 0, 0);
        metricLayout->setSpacing(0);

        titleLabel = new QLabel(title, metricWidget);
        titleLabel->setAlignment(Qt::AlignLeft);
        titleLabel->setStyleSheet("font-size: 9px; font-weight: 700;");

        valueLabel = new QLabel(value, metricWidget);
        valueLabel->setAlignment(Qt::AlignLeft);
        valueLabel->setStyleSheet("font-size: 18px; font-weight: 500;");

        metricLayout->addWidget(titleLabel);
        metricLayout->addWidget(valueLabel);
        return metricWidget;
    };

    statusLayout->addWidget(createStatusMetric(LanguageManager::text("metric_step"), stepMetricTitleLabel, stepCountLabel, "0", 46));
    statusLayout->addWidget(createStatusMetric(LanguageManager::text("metric_state"), stateMetricTitleLabel, machineStateLabel, "q0", 54));
    statusLayout->addWidget(createStatusMetric(LanguageManager::text("metric_mode"), modeMetricTitleLabel, statusLabel, LanguageManager::text("status_ready"), 76));

    statusLayout->addStretch();

    QFrame *statusControlFrame = new QFrame(statusGroup);
    statusControlFrame->setObjectName("statusControlFrame");
    QHBoxLayout *statusControlLayout = new QHBoxLayout(statusControlFrame);
    statusControlLayout->setContentsMargins(8, 4, 8, 4);
    statusControlLayout->setSpacing(6);

    stepButton = new QPushButton(LanguageManager::text("button_step"));
    runButton = new QPushButton(LanguageManager::text("button_run_start"));
    resetButton = new QPushButton(LanguageManager::text("button_reset"));

    // Nie ustawiaj stylów na sztywno; obsługuje je aktywny motyw.
    // Arkusze stylów motywu zajmują się wyglądem przycisków.

    speedLabel = new QLabel(LanguageManager::text("label_speed"));
    speedLabel->setStyleSheet("font-size: 11px; font-weight: 600;");
    speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setMinimum(100);
    speedSlider->setMaximum(2000);
    speedSlider->setValue(500);
    speedSlider->setFixedWidth(170);
    speedSlider->setFixedHeight(18);

    statusControlLayout->addWidget(speedLabel);
    statusControlLayout->addWidget(speedSlider);
    statusControlLayout->addSpacing(8);
    statusControlLayout->addWidget(stepButton);
    statusControlLayout->addWidget(runButton);
    statusControlLayout->addWidget(resetButton);
    statusLayout->addWidget(statusControlFrame);

    executorLayout->addWidget(statusGroup);

    // --- SEKCJA WEJŚCIA TAŚMY ---
    inputGroup = new QGroupBox(LanguageManager::text("group_input_tape"), executorWidget);
    QVBoxLayout *inputGroupLayout = new QVBoxLayout(inputGroup);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    tapeInputLabel = new QLabel(LanguageManager::text("label_tape"));
    inputLayout->addWidget(tapeInputLabel);
    tapeInputField = new QLineEdit();
    tapeInputField->setValidator(new QRegularExpressionValidator(QRegularExpression("[ -~]*"), this));
    tapeInputField->setPlaceholderText(LanguageManager::text("tape_placeholder"));
    inputLayout->addWidget(tapeInputField);

    inputGroupLayout->addLayout(inputLayout);

    tapeWarningLabel = new QLabel(LanguageManager::text("tape_warning"), inputGroup);
    tapeWarningLabel->setStyleSheet("color: red; font-weight: bold;");
    tapeWarningLabel->hide(); // domyślnie ukryte
    inputGroupLayout->addWidget(tapeWarningLabel);

    executorLayout->addWidget(inputGroup);

    // --- WIDOK TAŚMY ---
    tapeGroup = new QGroupBox(LanguageManager::text("group_tape_preview"), executorWidget);
    tapeGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    tapeGroup->setMaximumHeight(120);
    QVBoxLayout *tapeLayout = new QVBoxLayout(tapeGroup);
    tapeLayout->setContentsMargins(8, 6, 8, 8);
    tapeLayout->setSpacing(6);

    tapeScrollArea = new QScrollArea(tapeGroup);
    tapeScrollArea->setWidgetResizable(false);
    tapeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tapeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tapeScrollArea->setFrameShape(QFrame::NoFrame);
    tapeScrollArea->setFixedHeight(88);
    tapeScrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    QFrame *tapeFrame = new QFrame(tapeGroup);
    tapeFrame->setObjectName("tapeFrame");
    tapeFrame->setFrameShape(QFrame::NoFrame);
    tapeFrame->setFixedHeight(84);
    tapeFrame->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    QHBoxLayout *tapeFrameLayout = new QHBoxLayout(tapeFrame);
    tapeFrameLayout->setContentsMargins(8, 8, 8, 8);
    tapeFrameLayout->setSpacing(4);
    tapeFrameLayout->setSizeConstraint(QLayout::SetFixedSize);

    tapeDisplay = tapeFrame;
    setupTapeCells();
    tapeScrollArea->setWidget(tapeFrame);
    tapeLayout->addWidget(tapeScrollArea);
    executorLayout->addWidget(tapeGroup);

    // Podłącz sygnały.
    connect(tapeInputField, &QLineEdit::textChanged, this, &MachineEditorTab::onTapeInputChanged);
    connect(stepButton, &QPushButton::clicked, this, &MachineEditorTab::onStepClicked);
    connect(runButton, &QPushButton::clicked, this, &MachineEditorTab::onRunClicked);
    connect(resetButton, &QPushButton::clicked, this, &MachineEditorTab::onResetClicked);
    connect(speedSlider, QOverload<int>::of(&QSlider::valueChanged), this, &MachineEditorTab::onSpeedChanged);

    return executorWidget;
}

void MachineEditorTab::setStatusText(const char *key)
{
    currentStatusKey = QString::fromLatin1(key);
    currentStatusArgument.clear();
    updateStatusLabelText();
}

void MachineEditorTab::setStatusText(const char *key, const QString &argument)
{
    currentStatusKey = QString::fromLatin1(key);
    currentStatusArgument = argument;
    updateStatusLabelText();
}

void MachineEditorTab::updateStatusLabelText()
{
    if (!statusLabel || currentStatusKey.isEmpty())
    {
        return;
    }

    QString statusText = LanguageManager::text(currentStatusKey);
    if (!currentStatusArgument.isEmpty())
    {
        statusText = statusText.arg(currentStatusArgument);
    }

    statusLabel->setText(statusText);
}

void MachineEditorTab::retranslateTransitionMoves()
{
    if (!transitionsTable)
    {
        return;
    }

    QSignalBlocker blockTableSignals(transitionsTable);
    for (int row = 0; row < transitionsTable->rowCount(); ++row)
    {
        for (int column = 0; column < transitionsTable->columnCount(); ++column)
        {
            QTableWidgetItem *item = transitionsTable->item(row, column);
            if (!item)
            {
                continue;
            }

            QStringList parts = item->text().split(",");
            if (parts.size() != 3)
            {
                continue;
            }

            for (QString &part : parts)
            {
                part = part.trimmed();
            }

            parts[1] = LanguageManager::localizedMoveText(parts[1]);
            item->setText(QString("%1, %2, %3").arg(parts[0], parts[1], parts[2]));
        }
    }

    populateTransitionMapWithDefaults();
}

void MachineEditorTab::retranslateUi()
{
    if (logicEditorGroup) logicEditorGroup->setTitle(LanguageManager::text("group_logic_editor"));
    if (statesLabel) statesLabel->setText(LanguageManager::text("label_states"));
    if (alphabetLabel) alphabetLabel->setText(LanguageManager::text("label_alphabet"));
    if (blankLabel) blankLabel->setText(LanguageManager::text("label_blank"));
    if (initialLabel) initialLabel->setText(LanguageManager::text("label_start"));
    if (aiGenerateButton) aiGenerateButton->setToolTip(LanguageManager::text("ai_button_tooltip"));
    if (statusGroup) statusGroup->setTitle(LanguageManager::text("group_machine_state"));
    if (stepMetricTitleLabel) stepMetricTitleLabel->setText(LanguageManager::text("metric_step"));
    if (stateMetricTitleLabel) stateMetricTitleLabel->setText(LanguageManager::text("metric_state"));
    if (modeMetricTitleLabel) modeMetricTitleLabel->setText(LanguageManager::text("metric_mode"));
    if (stepButton) stepButton->setText(LanguageManager::text("button_step"));
    if (runButton) runButton->setText(LanguageManager::text(isRunning ? "button_stop" : "button_run_start"));
    if (resetButton) resetButton->setText(LanguageManager::text("button_reset"));
    if (speedLabel) speedLabel->setText(LanguageManager::text("label_speed"));
    if (inputGroup) inputGroup->setTitle(LanguageManager::text("group_input_tape"));
    if (tapeInputLabel) tapeInputLabel->setText(LanguageManager::text("label_tape"));
    if (tapeInputField) tapeInputField->setPlaceholderText(LanguageManager::text("tape_placeholder"));
    if (tapeWarningLabel) tapeWarningLabel->setText(LanguageManager::text("tape_warning"));
    if (tapeGroup) tapeGroup->setTitle(LanguageManager::text("group_tape_preview"));

    if (currentFilePath.isEmpty())
    {
        const QRegularExpression generatedTitleRegex("^(Maszyna|Machine)\\s+(\\d+)$");
        const QRegularExpressionMatch match = generatedTitleRegex.match(untitledTitle);
        if (match.hasMatch())
        {
            untitledTitle = LanguageManager::text("untitled_machine").arg(match.captured(2));
            emit titleChanged(tabTitle());
        }
    }

    updateStatusLabelText();
    retranslateTransitionMoves();
}
