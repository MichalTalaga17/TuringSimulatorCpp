#include "MachineEditorTab.h"
#include "TransitionDelegate.h"

#include <QAbstractSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QListView>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QRegularExpressionValidator>

MachineEditorTab::MachineEditorTab(const QString &untitledTitle, bool startWithExample, QWidget *parent)
    : QWidget(parent), numStatesSpinBox(nullptr), alphabetInputField(nullptr), blankSymbolField(nullptr), initialStateCombo(nullptr), aiGenerateButton(nullptr), transitionsTable(nullptr), machineStateLabel(nullptr), stepCountLabel(nullptr), tapeScrollArea(nullptr), tapeDisplay(nullptr), tapeInputField(nullptr), stepButton(nullptr), runButton(nullptr), resetButton(nullptr), speedSlider(nullptr), speedLabel(nullptr), statusLabel(nullptr), machine(nullptr), blankSymbol('_'), isRunning(false), executionSpeed(500), executionTimer(std::make_unique<QTimer>()), tapeDisplayRefreshTimer(std::make_unique<QTimer>()), untitledTitle(untitledTitle), startWithExample(startWithExample), highlightedTransitionRow(-1), highlightedTransitionCol(-1), aiGenerationInProgress(false)
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

    QGroupBox *logicEditorGroup = new QGroupBox("Edytor logiki", builderWidget);
    logicEditorGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *logicEditorLayout = new QVBoxLayout(logicEditorGroup);
    logicEditorLayout->setSpacing(10);

    QHBoxLayout *configFieldsLayout = new QHBoxLayout();
    configFieldsLayout->setSpacing(10);

    configFieldsLayout->addWidget(new QLabel("Stany:"));
    numStatesSpinBox = new QSpinBox();
    numStatesSpinBox->setObjectName("numStatesSpinBox");
    numStatesSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    numStatesSpinBox->setMinimum(1);
    numStatesSpinBox->setMaximum(20);
    numStatesSpinBox->setValue(2);
    numStatesSpinBox->setFixedHeight(31);
    numStatesSpinBox->setMaximumWidth(72);
    configFieldsLayout->addWidget(numStatesSpinBox);

    configFieldsLayout->addWidget(new QLabel("Alfabet:"));
    alphabetInputField = new QLineEdit();
    alphabetInputField->setPlaceholderText("0,1");
    // Ogranicz do standardowych drukowalnych znaków ASCII (32-126).
    alphabetInputField->setValidator(new QRegularExpressionValidator(QRegularExpression("[ -~]*"), this));
    alphabetInputField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    configFieldsLayout->addWidget(alphabetInputField, 1);

    configFieldsLayout->addWidget(new QLabel("Pusty:"));
    blankSymbolField = new QLineEdit();
    blankSymbolField->setText("_");
    // Ogranicz do jednego standardowego drukowalnego znaku ASCII bez spacji.
    blankSymbolField->setValidator(new QRegularExpressionValidator(QRegularExpression("[!-~]"), this));
    blankSymbolField->setMaximumWidth(60);
    configFieldsLayout->addWidget(blankSymbolField);

    configFieldsLayout->addWidget(new QLabel("Start:"));
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
    aiGenerateButton->setToolTip("Wygeneruj maszynę przez Gemini API.");
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
        if (rebuildMachineFromEditor() && statusLabel) {
            statusLabel->setText("Przebudowano");
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
    QGroupBox *statusGroup = new QGroupBox("Stan maszyny", executorWidget);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);
    statusLayout->setSpacing(6);

    auto createStatusMetric = [statusGroup](const QString &title, QLabel *&valueLabel, const QString &value, int minWidth)
    {
        QWidget *metricWidget = new QWidget(statusGroup);
        metricWidget->setMinimumWidth(minWidth);

        QVBoxLayout *metricLayout = new QVBoxLayout(metricWidget);
        metricLayout->setContentsMargins(0, 0, 0, 0);
        metricLayout->setSpacing(0);

        QLabel *titleLabel = new QLabel(title, metricWidget);
        titleLabel->setAlignment(Qt::AlignLeft);
        titleLabel->setStyleSheet("font-size: 9px; font-weight: 700;");

        valueLabel = new QLabel(value, metricWidget);
        valueLabel->setAlignment(Qt::AlignLeft);
        valueLabel->setStyleSheet("font-size: 18px; font-weight: 500;");

        metricLayout->addWidget(titleLabel);
        metricLayout->addWidget(valueLabel);
        return metricWidget;
    };

    statusLayout->addWidget(createStatusMetric("KROK", stepCountLabel, "0", 46));
    statusLayout->addWidget(createStatusMetric("STAN", machineStateLabel, "q0", 54));
    statusLayout->addWidget(createStatusMetric("TRYB", statusLabel, "Gotowe", 76));

    statusLayout->addStretch();

    QFrame *statusControlFrame = new QFrame(statusGroup);
    statusControlFrame->setObjectName("statusControlFrame");
    QHBoxLayout *statusControlLayout = new QHBoxLayout(statusControlFrame);
    statusControlLayout->setContentsMargins(8, 4, 8, 4);
    statusControlLayout->setSpacing(6);

     stepButton = new QPushButton("Krok");
    runButton = new QPushButton("Start");
    resetButton = new QPushButton("Resetuj");

    // Nie ustawiaj stylów na sztywno; obsługuje je aktywny motyw.
    // Arkusze stylów motywu zajmują się wyglądem przycisków.

    speedLabel = new QLabel("Szybkość:");
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
    QGroupBox *inputGroup = new QGroupBox("Taśma wejściowa", executorWidget);
    QVBoxLayout *inputGroupLayout = new QVBoxLayout(inputGroup);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Taśma:"));
    tapeInputField = new QLineEdit();
    tapeInputField->setValidator(new QRegularExpressionValidator(QRegularExpression("[ -~]*"), this));
    tapeInputField->setPlaceholderText("np. 1001");
    inputLayout->addWidget(tapeInputField);

    inputGroupLayout->addLayout(inputLayout);

    tapeWarningLabel = new QLabel("Taśma zawiera symbole spoza alfabetu. Wykonanie zatrzyma maszynę.", inputGroup);
    tapeWarningLabel->setStyleSheet("color: red; font-weight: bold;");
    tapeWarningLabel->hide(); // domyślnie ukryte
    inputGroupLayout->addWidget(tapeWarningLabel);

    executorLayout->addWidget(inputGroup);

    // --- WIDOK TAŚMY ---
    QGroupBox *tapeGroup = new QGroupBox("Podgląd taśmy", executorWidget);
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
