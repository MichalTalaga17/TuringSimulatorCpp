#include "ui.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QStyleFactory>
#include <QPalette>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScrollBar>
#include <QHeaderView>
#include <QTimer>
#include <QAbstractSpinBox>
#include <QListView>
#include <QSpinBox>
#include <algorithm>
#include <sstream>
#include <tuple>

namespace {

const QString& compactLogicComboStyle()
{
    static const QString style = R"(
        QComboBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 3px 6px;
            background-color: #ffffff;
            color: #202020;
            font-size: 11px;
        }
        QComboBox:hover {
            border-color: #a8c7e8;
            background-color: #f7fbff;
        }
        QComboBox:focus {
            border: 1px solid #0078d4;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 0;
            border: none;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: none;
            width: 0;
            height: 0;
        }
        QComboBox QAbstractItemView {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            background-color: #ffffff;
            color: #202020;
            outline: 0;
            selection-background-color: #eaf4ff;
            selection-color: #005a9e;
        }
        QComboBox QAbstractItemView::item {
            min-height: 24px;
            padding: 4px 8px;
        }
        QComboBox QAbstractItemView::item:hover,
        QComboBox QAbstractItemView::item:selected {
            background-color: #eaf4ff;
            color: #005a9e;
        }
    )";
    return style;
}

void applyCompactLogicComboStyle(QComboBox *combo, int popupMinimumWidth = 64)
{
    combo->setStyleSheet(compactLogicComboStyle());

    QListView *popupView = new QListView(combo);
    popupView->setMinimumWidth(popupMinimumWidth);
    popupView->setTextElideMode(Qt::ElideNone);
    popupView->setFrameShape(QFrame::NoFrame);
    popupView->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    popupView->setAttribute(Qt::WA_TranslucentBackground);
    popupView->setAutoFillBackground(false);
    popupView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    popupView->viewport()->setAutoFillBackground(false);
    popupView->setStyleSheet(R"(
        QListView {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            background-color: #ffffff;
            color: #202020;
            outline: 0;
            padding: 2px;
        }
        QListView::item {
            min-height: 24px;
            padding: 4px 8px;
            border-radius: 4px;
        }
        QListView::item:hover,
        QListView::item:selected {
            background-color: #eaf4ff;
            color: #005a9e;
        }
    )");
    combo->setView(popupView);
}

} // namespace

TuringSimulatorUI::TuringSimulatorUI(QWidget *parent)
    : QMainWindow(parent)
    , numStatesSpinBox(nullptr)
    , alphabetInputField(nullptr)
    , blankSymbolField(nullptr)
    , initialStateCombo(nullptr)
    , transitionsTable(nullptr)
    , machineStateLabel(nullptr)
    , stepCountLabel(nullptr)
    , tapeScrollArea(nullptr)
    , tapeDisplay(nullptr)
    , tapeInputField(nullptr)
    , stepButton(nullptr)
    , runButton(nullptr)
    , resetButton(nullptr)
    , speedSlider(nullptr)
    , speedLabel(nullptr)
    , statusLabel(nullptr)
    , machine(nullptr)
    , blankSymbol('_')
    , isRunning(false)
    , executionSpeed(500)
    , executionTimer(std::make_unique<QTimer>())
    , tapeDisplayRefreshTimer(std::make_unique<QTimer>())
{
    setupUI();
    applyModernStyle();

    // Connect timer for execution
    connect(executionTimer.get(), &QTimer::timeout, this, &TuringSimulatorUI::onExecutionTimer);
    tapeDisplayRefreshTimer->setSingleShot(true);
    connect(tapeDisplayRefreshTimer.get(), &QTimer::timeout, this, &TuringSimulatorUI::updateTapeDisplay);

    scheduleTapeDisplayRefresh();
}

void TuringSimulatorUI::setupUI()
{
    setWindowTitle("Turing Machine Simulator");
    setGeometry(100, 100, 1200, 850);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *pageScrollArea = new QScrollArea(centralWidget);
    pageScrollArea->setWidgetResizable(true);
    pageScrollArea->setFrameShape(QFrame::NoFrame);
    pageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *pageWidget = new QWidget(pageScrollArea);
    QVBoxLayout *pageLayout = new QVBoxLayout(pageWidget);
    pageLayout->setContentsMargins(16, 16, 16, 16);
    pageLayout->setSpacing(12);

    // --- Executor Panel FIRST (so statusLabel is created early) ---
    pageLayout->addWidget(setupExecutorTab(), 0);

    // --- Builder Panel SECOND (can safely use statusLabel) ---
    pageLayout->addWidget(setupBuilderTab(), 1);

    pageScrollArea->setWidget(pageWidget);
    mainLayout->addWidget(pageScrollArea);
    setCentralWidget(centralWidget);
}

void TuringSimulatorUI::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (!machine || !tapeScrollArea || tapeCells.empty()) {
        return;
    }

    scheduleTapeDisplayRefresh();
}

QWidget* TuringSimulatorUI::setupBuilderTab()
{
    QWidget *builderWidget = new QWidget();
    builderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *builderLayout = new QVBoxLayout(builderWidget);
    builderLayout->setSpacing(12);
    builderLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox *logicEditorGroup = new QGroupBox("Logic Editor", builderWidget);
    logicEditorGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *logicEditorLayout = new QVBoxLayout(logicEditorGroup);
    logicEditorLayout->setSpacing(10);

    QHBoxLayout *configFieldsLayout = new QHBoxLayout();
    configFieldsLayout->setSpacing(10);

    configFieldsLayout->addWidget(new QLabel("States:"));
    numStatesSpinBox = new QSpinBox();
    numStatesSpinBox->setObjectName("numStatesSpinBox");
    numStatesSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    numStatesSpinBox->setMinimum(1);
    numStatesSpinBox->setMaximum(20);
    numStatesSpinBox->setValue(2);
    numStatesSpinBox->setFixedHeight(31);
    numStatesSpinBox->setMaximumWidth(72);
    configFieldsLayout->addWidget(numStatesSpinBox);

    configFieldsLayout->addWidget(new QLabel("Alphabet:"));
    alphabetInputField = new QLineEdit();
    alphabetInputField->setPlaceholderText("0,1");
    alphabetInputField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    configFieldsLayout->addWidget(alphabetInputField, 1);

    configFieldsLayout->addWidget(new QLabel("Blank:"));
    blankSymbolField = new QLineEdit();
    blankSymbolField->setText("_");
    blankSymbolField->setMaximumWidth(60);
    configFieldsLayout->addWidget(blankSymbolField);

    configFieldsLayout->addWidget(new QLabel("Initial:"));
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

    logicEditorLayout->addLayout(configFieldsLayout);

    transitionsTable = new QTableWidget();
    transitionsTable->setObjectName("transitionsTable");
    transitionsTable->setFrameShape(QFrame::NoFrame);
    transitionsTable->setShowGrid(false);
    transitionsTable->setAlternatingRowColors(true);
    transitionsTable->setWordWrap(false);
    transitionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    transitionsTable->setFocusPolicy(Qt::NoFocus);
    transitionsTable->setSelectionMode(QAbstractItemView::NoSelection);
    transitionsTable->setMinimumHeight(300);
    transitionsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    logicEditorLayout->addWidget(transitionsTable, 1);

    builderLayout->addWidget(logicEditorGroup, 1);

    // Connect signals (after all widgets are created)
    connect(numStatesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TuringSimulatorUI::onNumStatesChanged);
    connect(alphabetInputField, &QLineEdit::textChanged, this, &TuringSimulatorUI::onAlphabetChanged);
    connect(blankSymbolField, &QLineEdit::textChanged, this, &TuringSimulatorUI::onBlankSymbolChanged);
    connect(initialStateCombo, &QComboBox::currentTextChanged, this, [this](const QString& state) {
        initialState = state.toStdString();
        if (rebuildMachineFromEditor() && statusLabel) {
            statusLabel->setText("Rebuilt");
        }
    });

    // Initialize with safe blocking
    transitionsTable->blockSignals(true);
    onNumStatesChanged(2);
    transitionsTable->blockSignals(false);
    applyDefaultExample();

    return builderWidget;
}

QWidget* TuringSimulatorUI::setupExecutorTab()
{
    QWidget *executorWidget = new QWidget();
    executorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QVBoxLayout *executorLayout = new QVBoxLayout(executorWidget);
    executorLayout->setSpacing(12);

    // --- Status Panel ---
    QGroupBox *statusGroup = new QGroupBox("Machine Status", executorWidget);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);
    statusLayout->setSpacing(6);

    auto createStatusMetric = [statusGroup](const QString& title, QLabel *&valueLabel, const QString& value, int minWidth) {
        QWidget *metricWidget = new QWidget(statusGroup);
        metricWidget->setMinimumWidth(minWidth);

        QVBoxLayout *metricLayout = new QVBoxLayout(metricWidget);
        metricLayout->setContentsMargins(0, 0, 0, 0);
        metricLayout->setSpacing(0);

        QLabel *titleLabel = new QLabel(title, metricWidget);
        titleLabel->setAlignment(Qt::AlignLeft);
        titleLabel->setStyleSheet("color: #7a7a7a; font-size: 9px; font-weight: 700;");

        valueLabel = new QLabel(value, metricWidget);
        valueLabel->setAlignment(Qt::AlignLeft);
        valueLabel->setStyleSheet("color: #202020; font-size: 18px; font-weight: 500;");

        metricLayout->addWidget(titleLabel);
        metricLayout->addWidget(valueLabel);
        return metricWidget;
    };

    statusLayout->addWidget(createStatusMetric("STEP", stepCountLabel, "0", 46));
    statusLayout->addWidget(createStatusMetric("STATE", machineStateLabel, "q0", 54));
    statusLayout->addWidget(createStatusMetric("STATUS", statusLabel, "Ready", 76));

    statusLayout->addStretch();

    QFrame *statusControlFrame = new QFrame(statusGroup);
    statusControlFrame->setObjectName("statusControlFrame");
    QHBoxLayout *statusControlLayout = new QHBoxLayout(statusControlFrame);
    statusControlLayout->setContentsMargins(8, 4, 8, 4);
    statusControlLayout->setSpacing(6);

    stepButton = new QPushButton("Step");
    runButton = new QPushButton("Run");
    resetButton = new QPushButton("Reset");

    const QString machineControlButtonStyle = R"(
        QPushButton {
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
        QPushButton:hover {
            background-color: #eaf4ff;
            border-color: #0078d4;
            color: #005a9e;
        }
        QPushButton:pressed {
            background-color: #d8ecff;
            border-color: #005a9e;
        }
        QPushButton:disabled {
            background-color: #f0f0f0;
            border-color: #dddddd;
            color: #888888;
        }
    )";
    stepButton->setStyleSheet(machineControlButtonStyle);
    runButton->setStyleSheet(machineControlButtonStyle);
    resetButton->setStyleSheet(machineControlButtonStyle);

    speedLabel = new QLabel("Speed:");
    speedLabel->setStyleSheet("font-size: 11px; font-weight: 600; color: #505050;");
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

    // --- Tape Input Section ---
    QGroupBox *inputGroup = new QGroupBox("Input Tape", executorWidget);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputGroup);
    inputLayout->addWidget(new QLabel("Tape:"));
    tapeInputField = new QLineEdit();
    tapeInputField->setPlaceholderText("e.g., 1001");
    inputLayout->addWidget(tapeInputField);
    executorLayout->addWidget(inputGroup);

    // --- Tape Display ---
    QGroupBox *tapeGroup = new QGroupBox("Tape Visualization", executorWidget);
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

    // Connect signals
    connect(tapeInputField, &QLineEdit::textChanged, this, &TuringSimulatorUI::onTapeInputChanged);
    connect(stepButton, &QPushButton::clicked, this, &TuringSimulatorUI::onStepClicked);
    connect(runButton, &QPushButton::clicked, this, &TuringSimulatorUI::onRunClicked);
    connect(resetButton, &QPushButton::clicked, this, &TuringSimulatorUI::onResetClicked);
    connect(speedSlider, QOverload<int>::of(&QSlider::valueChanged), this, &TuringSimulatorUI::onSpeedChanged);

    return executorWidget;
}

void TuringSimulatorUI::applyModernStyle()
{
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
        QGroupBox {
            color: #202020;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            margin-top: 8px;
            padding-top: 8px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 3px 0 3px;
        }
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 11px;
            font-weight: bold;
            outline: none;
            min-width: 70px;
            min-height: 32px;
        }
        QPushButton:hover {
            background-color: #106ebe;
        }
        QPushButton:pressed {
            background-color: #005a9e;
        }
        QPushButton:focus {
            outline: 2px solid #0078d4;
            outline-offset: 2px;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #808080;
        }
        QLineEdit, QTextEdit, QComboBox, QSpinBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 6px 8px;
            background-color: white;
            color: #202020;
            font-size: 11px;
            selection-background-color: #cfe8ff;
        }
        QLineEdit:hover, QTextEdit:hover, QComboBox:hover, QSpinBox:hover {
            border-color: #a8c7e8;
            background-color: #f7fbff;
        }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus {
            border: 2px solid #0078d4;
            background-color: white;
        }
        QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button {
            border: none;
            border-left: 1px solid #e0e0e0;
            background-color: #f7f7f7;
            width: 20px;
        }
        QComboBox::drop-down {
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }
        QSpinBox::up-button {
            border-top-right-radius: 6px;
        }
        QSpinBox::down-button {
            border-bottom-right-radius: 6px;
        }
        QSpinBox#numStatesSpinBox {
            padding-right: 8px;
        }
        QSpinBox#numStatesSpinBox::up-button,
        QSpinBox#numStatesSpinBox::down-button {
            width: 0;
            border: none;
            background: transparent;
        }
        QComboBox#initialStateCombo {
            padding-right: 8px;
        }
        QComboBox#initialStateCombo::drop-down {
            width: 0;
            border: none;
            background: transparent;
        }
        QComboBox#initialStateCombo::down-arrow {
            image: none;
            width: 0;
            height: 0;
        }
        QComboBox QAbstractItemView {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            background-color: #ffffff;
            color: #202020;
            outline: 0;
            selection-background-color: #eaf4ff;
            selection-color: #005a9e;
        }
        QComboBox QAbstractItemView::item {
            min-height: 24px;
            padding: 4px 8px;
        }
        QComboBox QAbstractItemView::item:hover,
        QComboBox QAbstractItemView::item:selected {
            background-color: #eaf4ff;
            color: #005a9e;
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
}

// === Builder Slots ===

void TuringSimulatorUI::onNumStatesChanged(int value)
{
    states.clear();
    for (int i = 0; i < value; ++i) {
        states.push_back("q" + std::to_string(i));
    }

    // Update combos with signal blocking
    initialStateCombo->blockSignals(true);
    initialStateCombo->clear();

    for (const auto& state : states) {
        initialStateCombo->addItem(QString::fromStdString(state));
    }

    if (initialStateCombo->count() > 0) {
        initialStateCombo->setCurrentIndex(0);
        initialState = states[0];
    }
    initialStateCombo->blockSignals(false);

    // Update only the toState combos if table already exists with same column count
    if (transitionsTable->rowCount() > 0 && transitionsTable->columnCount() > 0) {
        transitionsTable->blockSignals(true);

        int oldRowCount = transitionsTable->rowCount();
        int newRowCount = static_cast<int>(states.size());

        // Set row count FIRST
        transitionsTable->setRowCount(newRowCount);

        // THEN update vertical headers (state names) after row count is set
        QStringList stateLabels;
        for (const auto& state : states) {
            stateLabels << QString::fromStdString(state);
        }
        transitionsTable->setVerticalHeaderLabels(stateLabels);

        // If rows were added, create cells for new rows
        if (newRowCount > oldRowCount) {
            for (int row = oldRowCount; row < newRowCount; ++row) {
                transitionsTable->setRowHeight(row, 38);
                for (int col = 0; col < transitionsTable->columnCount(); ++col) {
                    transitionsTable->setCellWidget(row, col, createTransitionCell(row, col));
                }
            }
        } else {
            // Rows were removed or stayed same, update all existing rows
            for (int row = 0; row < newRowCount; ++row) {
                transitionsTable->setRowHeight(row, 38);
                for (int col = 0; col < transitionsTable->columnCount(); ++col) {
                    QWidget *cellWidget = transitionsTable->cellWidget(row, col);
                    if (!cellWidget) continue;

                    QComboBox *toStateCombo = cellWidget->findChild<QComboBox*>(QString("tostate_%1_%2").arg(row).arg(col));
                    if (toStateCombo) {
                        toStateCombo->blockSignals(true);
                        int currentIdx = toStateCombo->currentIndex();
                        toStateCombo->clear();
                        for (const auto& state : states) {
                            toStateCombo->addItem(QString::fromStdString(state));
                        }
                        if (currentIdx >= 0 && currentIdx < toStateCombo->count()) {
                            toStateCombo->setCurrentIndex(currentIdx);
                        }
                        toStateCombo->blockSignals(false);
                    }
                }
            }
        }

        transitionsTable->blockSignals(false);

        // Repopulate transition map with updated defaults
        populateTransitionMapWithDefaults();
    } else {
        // First time: rebuild the entire table
        rebuildTransitionsTable();
    }

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel) {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void TuringSimulatorUI::onAlphabetChanged()
{
    QString alphaStr = alphabetInputField->text();
    std::set<char> newAlphabet;

    // Get current blank symbol
    QString blankStr = blankSymbolField->text();
    char currentBlank = blankStr.isEmpty() || blankStr.length() != 1 ? '_' : blankStr[0].toLatin1();

    // Parse alphabet (comma or space separated, or just chars)
    for (int i = 0; i < alphaStr.length(); ++i) {
        char c = alphaStr[i].toLatin1();
        if (c != ',' && c != ' ' && c != 0) {
            newAlphabet.insert(c);
        }
    }

    // Always add the blank symbol as first column (if not already in alphabet)
    newAlphabet.insert(currentBlank);

    // Only rebuild if alphabet changed
    if (newAlphabet != alphabet) {
        alphabet = newAlphabet;
        rebuildTransitionsTable();
        // Note: transitionMap is populated by rebuildTransitionsTable() via populateTransitionMapWithDefaults()
    }

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel) {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void TuringSimulatorUI::onBlankSymbolChanged()
{
    QString blankStr = blankSymbolField->text();
    char newBlank = blankStr.isEmpty() || blankStr.length() != 1 ? '_' : blankStr[0].toLatin1();

    // Check if blank symbol is already in alphabet
    bool blankInAlphabet = alphabet.find(newBlank) != alphabet.end();

    if (!blankInAlphabet && !alphabet.empty()) {
        // Blank symbol changed and it's not in the current alphabet, rebuild table
        std::set<char> newAlphabet = alphabet;

        // Add new blank symbol
        newAlphabet.insert(newBlank);

        alphabet = newAlphabet;
        rebuildTransitionsTable();
    }

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel) {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void TuringSimulatorUI::onCellComboChanged()
{
    // Sender is a QComboBox inside a cell
    QComboBox *senderCombo = qobject_cast<QComboBox*>(sender());
    if (!senderCombo) return;

    // Use the object name to directly determine row/col instead of searching
    QString objName = senderCombo->objectName();
    int row = -1, col = -1;

    // Parse name like "write_0_1" or "move_0_1" or "tostate_0_1"
    QStringList parts = objName.split('_');
    if (parts.size() >= 3) {
        row = parts[1].toInt();
        col = parts[2].toInt();
    }

    if (row < 0 || col < 0 || row >= transitionsTable->rowCount() || col >= transitionsTable->columnCount()) {
        return;
    }

    QWidget *cellWidget = transitionsTable->cellWidget(row, col);
    if (!cellWidget) return;

    // Get the 3 combos from cache or find them
    auto cacheKey = std::make_pair(row, col);
    QComboBox *writeCombo = nullptr, *moveCombo = nullptr, *toStateCombo = nullptr;

    auto it = comboCache.find(cacheKey);
    if (it != comboCache.end()) {
        std::tie(writeCombo, moveCombo, toStateCombo) = it->second;
    } else {
        writeCombo = cellWidget->findChild<QComboBox*>(QString("write_%1_%2").arg(row).arg(col));
        moveCombo = cellWidget->findChild<QComboBox*>(QString("move_%1_%2").arg(row).arg(col));
        toStateCombo = cellWidget->findChild<QComboBox*>(QString("tostate_%1_%2").arg(row).arg(col));

        if (writeCombo && moveCombo && toStateCombo) {
            comboCache[cacheKey] = std::make_tuple(writeCombo, moveCombo, toStateCombo);
        }
    }

    if (!writeCombo || !moveCombo || !toStateCombo) return;

    // Build transition string
    QString write = writeCombo->currentText();
    QString move = moveCombo->currentText();
    QString toState = toStateCombo->currentText();

    if (!write.isEmpty() && !move.isEmpty() && !toState.isEmpty()) {
        // Get the symbol for this column by index
        std::string state = states[row];
        char symbol = '0';
        int idx = 0;
        for (char s : alphabet) {
            if (idx == col) {
                symbol = s;
                break;
            }
            idx++;
        }

        // Store in map
        transitionMap[{state, symbol}] = {write[0].toLatin1(), move.toStdString(), toState.toStdString()};
    } else {
        // Remove from map if incomplete
        if (row < static_cast<int>(states.size()) && col < static_cast<int>(alphabet.size())) {
            std::string state = states[row];
            char symbol = '0';
            int idx = 0;
            for (char s : alphabet) {
                if (idx == col) {
                    symbol = s;
                    break;
                }
                idx++;
            }
            transitionMap.erase({state, symbol});
        }
    }

    if (rebuildMachineFromEditor() && statusLabel) {
        statusLabel->setText("Rebuilt");
    }
}

bool TuringSimulatorUI::buildMachineFromCurrentConfiguration(bool showSuccessDialog)
{
    auto showValidationWarning = [this, showSuccessDialog](const QString& message) {
        if (showSuccessDialog) {
            QMessageBox::warning(this, "Validation Error", message);
        }
    };

    // Validation
    if (states.empty()) {
        showValidationWarning("Please set the number of states");
        return false;
    }

    if (alphabet.empty()) {
        showValidationWarning("Please define the alphabet");
        return false;
    }

    if (initialState.empty()) {
        showValidationWarning("Please select an initial state");
        return false;
    }

    if (transitionMap.empty()) {
        showValidationWarning("Please define at least one transition");
        return false;
    }

    // Extract blank symbol
    QString blankStr = blankSymbolField->text();
    if (blankStr.isEmpty() || blankStr.length() != 1) {
        blankSymbol = '_';
    } else {
        blankSymbol = blankStr[0].toLatin1();
    }

    // Build machine
    machine = std::make_unique<TuringMachine>(blankSymbol);
    machine->setInitialState(initialState);

    // Add transitions from map
    for (const auto& [key, value] : transitionMap) {
        std::string fromState = key.first;
        char readSymbol = key.second;

        char writeSymbol = std::get<0>(value);
        std::string moveStr = std::get<1>(value);
        std::string toState = std::get<2>(value);

        HeadMove move = HeadMove::Stay;
        if (moveStr == "L") move = HeadMove::L;
        else if (moveStr == "R") move = HeadMove::R;
        else if (moveStr == "Stop") move = HeadMove::STOP;

        machine->addTransition(fromState, readSymbol, {writeSymbol, move, toState});
    }

    if (showSuccessDialog) {
        statusLabel->setText("Built");
        QMessageBox::information(this, "Success", "Machine built successfully! You can run it above.");
    }

    return true;
}

bool TuringSimulatorUI::rebuildMachineFromEditor()
{
    if (isRunning) {
        isRunning = false;
        executionTimer->stop();
        runButton->setText("Run");
        stepButton->setEnabled(true);
    }

    if (!buildMachineFromCurrentConfiguration(false) || !machine) {
        return false;
    }

    machine->loadTape(tapeInputField->text().trimmed().toStdString());
    updateDisplay();
    return true;
}

// === Executor Slots ===

void TuringSimulatorUI::onTapeInputChanged()
{
    if (rebuildMachineFromEditor() && statusLabel) {
        statusLabel->setText("Rebuilt");
    }
}

void TuringSimulatorUI::onStepClicked()
{
    if (!machine) {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    if (machine->step()) {
        updateDisplay();
    } else {
        statusLabel->setText(machine->isAccepted() ? "Accepted" : "Halted");
    }
}

void TuringSimulatorUI::onRunClicked()
{
    if (!machine) {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    isRunning = !isRunning;
    runButton->setText(isRunning ? "Stop" : "Run");
    stepButton->setEnabled(!isRunning);

    if (isRunning) {
        statusLabel->setText("Running");
        executionTimer->start(2001 - executionSpeed);
    } else {
        statusLabel->setText("Paused");
        executionTimer->stop();
    }
}

void TuringSimulatorUI::onResetClicked()
{
    if (machine) {
        machine->reset();
        isRunning = false;
        runButton->setText("Run");
        stepButton->setEnabled(true);
        executionTimer->stop();
        updateDisplay();
        statusLabel->setText("Reset");
    }
}

void TuringSimulatorUI::onSpeedChanged(int value)
{
    executionSpeed = value;
    if (isRunning) {
        executionTimer->setInterval(2001 - value);
    }
}

void TuringSimulatorUI::onExecutionTimer()
{
    if (machine && isRunning) {
        if (!machine->step()) {
            isRunning = false;
            runButton->setText("Run");
            stepButton->setEnabled(true);
            executionTimer->stop();
            statusLabel->setText(machine->isAccepted() ? "Accepted" : "Halted");
        }
        updateDisplay();
    }
}

// === Display Updates ===

void TuringSimulatorUI::updateDisplay()
{
    updateStateDisplay();
    updateTapeDisplay();
}

void TuringSimulatorUI::updateStateDisplay()
{
    if (machine) {
        machineStateLabel->setText(QString::fromStdString(machine->currentState()));
        stepCountLabel->setText(QString::fromStdString(std::to_string(machine->stepCount())));
    }
}

void TuringSimulatorUI::updateTapeDisplay()
{
    if (!machine || tapeCells.empty() || !tapeScrollArea) {
        return;
    }

    const Tape& tape = machine->tape();
    const std::size_t activeCellIndex = tapeCells.size() / 2;
    const int headPos = tape.headPosition();
    const int from = headPos - static_cast<int>(activeCellIndex);

    for (std::size_t index = 0; index < tapeCells.size(); ++index) {
        const int tapePosition = from + static_cast<int>(index);
        const char symbol = tape.snapshot(tapePosition, tapePosition).front();
        QLabel *cell = tapeCells[index];

        cell->setText(QString(symbol));

        if (index == activeCellIndex) {
            cell->setStyleSheet(
                "background-color: #eaf4ff;"
                "border: 2px solid #0078d4;"
                "border-radius: 10px;"
                "color: #005a9e;"
                "font-size: 22px;"
                "font-weight: 700;"
            );
        } else {
            cell->setStyleSheet(
                "background-color: #ffffff;"
                "border: 1px solid #d0d0d0;"
                "border-radius: 10px;"
                "color: #202020;"
                "font-size: 20px;"
                "font-weight: 600;"
            );
        }
    }

    QLabel *activeCell = tapeCells[activeCellIndex];
    const int viewportWidth = tapeScrollArea->viewport()->width();
    if (viewportWidth <= 0) {
        scheduleTapeDisplayRefresh();
        return;
    }

    const int targetValue = activeCell->x() + (activeCell->width() / 2) - (viewportWidth / 2);
    tapeScrollArea->horizontalScrollBar()->setValue(std::max(0, targetValue));
}

void TuringSimulatorUI::scheduleTapeDisplayRefresh()
{
    if (tapeDisplayRefreshTimer->isActive()) {
        return;
    }

    tapeDisplayRefreshTimer->start(0);
}

void TuringSimulatorUI::setupTapeCells()
{
    tapeCells.clear();

    auto *layout = qobject_cast<QHBoxLayout*>(tapeDisplay->layout());
    if (!layout) {
        return;
    }

    constexpr int visibleCellCount = 41;
    constexpr int activeCellIndex = visibleCellCount / 2;

    for (int index = 0; index < visibleCellCount; ++index) {
        QLabel *cell = new QLabel("-", tapeDisplay);
        cell->setAlignment(Qt::AlignCenter);
        cell->setFixedSize(46, 58);

        if (index == activeCellIndex) {
            cell->setStyleSheet(
                "background-color: #eaf4ff;"
                "border: 2px solid #0078d4;"
                "border-radius: 10px;"
                "color: #005a9e;"
                "font-size: 22px;"
                "font-weight: 700;"
            );
        } else {
            cell->setStyleSheet(
                "background-color: #ffffff;"
                "border: 1px solid #d0d0d0;"
                "border-radius: 10px;"
                "color: #202020;"
                "font-size: 20px;"
                "font-weight: 600;"
            );
        }

        layout->addWidget(cell);
        tapeCells.push_back(cell);
    }
}

void TuringSimulatorUI::applyDefaultExample()
{
    numStatesSpinBox->setValue(1);
    alphabetInputField->setText("01");
    blankSymbolField->setText("_");
    tapeInputField->setText("100010111");

    auto findColumnForSymbol = [this](char symbol) -> int {
        int columnIndex = 0;
        for (char alphabetSymbol : alphabet) {
            if (alphabetSymbol == symbol) {
                return columnIndex;
            }
            ++columnIndex;
        }
        return -1;
    };

    const int row = 0;
    const int blankColumn = findColumnForSymbol('_');
    const int zeroColumn = findColumnForSymbol('0');
    const int oneColumn = findColumnForSymbol('1');

    auto setTransitionCell = [this, row](int column, char writeSymbol, const QString& moveText, const QString& nextStateText) {
        if (column < 0) {
            return;
        }

        const auto cacheIt = comboCache.find({row, column});
        if (cacheIt == comboCache.end()) {
            return;
        }

        QComboBox *writeCombo = std::get<0>(cacheIt->second);
        QComboBox *moveCombo = std::get<1>(cacheIt->second);
        QComboBox *toStateCombo = std::get<2>(cacheIt->second);

        writeCombo->setCurrentText(QString(writeSymbol));
        moveCombo->setCurrentText(moveText);
        toStateCombo->setCurrentText(nextStateText);
    };

    setTransitionCell(zeroColumn, '1', "R", "q0");
    setTransitionCell(oneColumn, '0', "R", "q0");
    setTransitionCell(blankColumn, '_', "Stop", "q0");

    populateTransitionMapWithDefaults();

    if (rebuildMachineFromEditor()) {
        statusLabel->setText("Ready");
    }
}

QWidget* TuringSimulatorUI::createTransitionCell(int row, int col)
{
    QWidget *cellWidget = new QWidget();
    cellWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *layout = new QHBoxLayout(cellWidget);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(1);

    QComboBox *writeCombo = new QComboBox();
    writeCombo->setObjectName(QString("write_%1_%2").arg(row).arg(col));
    writeCombo->setMinimumHeight(28);
    writeCombo->setMaximumHeight(28);
    writeCombo->setFixedWidth(36);
    for (char c : alphabet) {
        writeCombo->addItem(QString(c));
    }
    applyCompactLogicComboStyle(writeCombo, 56);
    layout->addWidget(writeCombo);

    QComboBox *moveCombo = new QComboBox();
    moveCombo->setObjectName(QString("move_%1_%2").arg(row).arg(col));
    moveCombo->setMinimumHeight(28);
    moveCombo->setMaximumHeight(28);
    moveCombo->setFixedWidth(40);
    moveCombo->addItems({"L", "R", "Stay", "Stop"});
    applyCompactLogicComboStyle(moveCombo, 72);
    layout->addWidget(moveCombo);

    QComboBox *toStateCombo = new QComboBox();
    toStateCombo->setObjectName(QString("tostate_%1_%2").arg(row).arg(col));
    toStateCombo->setMinimumHeight(28);
    toStateCombo->setMaximumHeight(28);
    toStateCombo->setFixedWidth(36);
    for (const auto& state : states) {
        toStateCombo->addItem(QString::fromStdString(state));
    }
    applyCompactLogicComboStyle(toStateCombo, 64);
    layout->addWidget(toStateCombo);

    comboCache[{row, col}] = std::make_tuple(writeCombo, moveCombo, toStateCombo);
    setCellDefaults(row, col, writeCombo, moveCombo, toStateCombo);

    connect(writeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
    connect(moveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
    connect(toStateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);

    return cellWidget;
}

void TuringSimulatorUI::rebuildTransitionsTable()
{
    transitionsTable->blockSignals(true);
    clearTableCache();
    transitionsTable->clear();

    if (states.empty() || alphabet.empty()) {
        transitionsTable->blockSignals(false);
        return;
    }

    int numRows = static_cast<int>(states.size());
    int numCols = static_cast<int>(alphabet.size());
    transitionsTable->setRowCount(numRows);
    transitionsTable->setColumnCount(numCols);

    // Set vertical headers (state names)
    QStringList stateLabels;
    for (const auto& state : states) {
        stateLabels << QString::fromStdString(state);
    }
    transitionsTable->setVerticalHeaderLabels(stateLabels);

    // Set horizontal headers (alphabet symbols)
    QStringList alphabetLabels;
    for (char symbol : alphabet) {
        alphabetLabels << QString(symbol);
    }
    transitionsTable->setHorizontalHeaderLabels(alphabetLabels);

    // Create cells with embedded combo boxes
    for (int row = 0; row < numRows; ++row) {
        transitionsTable->setRowHeight(row, 38);
        for (int col = 0; col < numCols; ++col) {
            transitionsTable->setCellWidget(row, col, createTransitionCell(row, col));
        }
    }

    transitionsTable->blockSignals(false);

    // Populate transition map with default values
    populateTransitionMapWithDefaults();
}

void TuringSimulatorUI::populateTransitionMapWithDefaults()
{
    // Populate the transition map with default values for all cells
    // Defaults: write = read symbol, move = L, state = current state

    transitionMap.clear();

    int colIdx = 0;
    for (char readSymbol : alphabet) {
        for (int row = 0; row < static_cast<int>(states.size()); ++row) {
            QWidget *cellWidget = transitionsTable->cellWidget(row, colIdx);
            if (!cellWidget) continue;

            QComboBox *writeCombo = cellWidget->findChild<QComboBox*>(QString("write_%1_%2").arg(row).arg(colIdx));
            QComboBox *moveCombo = cellWidget->findChild<QComboBox*>(QString("move_%1_%2").arg(row).arg(colIdx));
            QComboBox *toStateCombo = cellWidget->findChild<QComboBox*>(QString("tostate_%1_%2").arg(row).arg(colIdx));

            if (writeCombo && moveCombo && toStateCombo) {
                QString write = writeCombo->currentText();
                QString move = moveCombo->currentText();
                QString toState = toStateCombo->currentText();

                if (!write.isEmpty() && !move.isEmpty() && !toState.isEmpty()) {
                    transitionMap[{states[row], readSymbol}] = {
                        write[0].toLatin1(),
                        move.toStdString(),
                        toState.toStdString()
                    };
                }
            }
        }
        colIdx++;
    }
}

void TuringSimulatorUI::clearTableCache()
{
    comboCache.clear();
}

void TuringSimulatorUI::setCellDefaults(int row, int col, QComboBox *writeCombo, QComboBox *moveCombo, QComboBox *toStateCombo)
{
    if (!writeCombo || !moveCombo || !toStateCombo) return;

    // Block signals during default setting
    writeCombo->blockSignals(true);
    moveCombo->blockSignals(true);
    toStateCombo->blockSignals(true);

    // Get the read symbol for this column
    char readSymbol = '0';
    int idx = 0;
    for (char s : alphabet) {
        if (idx == col) {
            readSymbol = s;
            break;
        }
        idx++;
    }

    // Set write combo to read symbol (default: write = read)
    int writeIdx = writeCombo->findText(QString(readSymbol));
    if (writeIdx >= 0) {
        writeCombo->setCurrentIndex(writeIdx);
    }

    // Set move combo to "L" (default: always left)
    int moveIdx = moveCombo->findText("L");
    if (moveIdx >= 0) {
        moveCombo->setCurrentIndex(moveIdx);
    }

    // Set state combo to current state (default: stay in same state)
    if (row < static_cast<int>(states.size())) {
        int stateIdx = toStateCombo->findText(QString::fromStdString(states[row]));
        if (stateIdx >= 0) {
            toStateCombo->setCurrentIndex(stateIdx);
        }
    }

    // Unblock signals
    writeCombo->blockSignals(false);
    moveCombo->blockSignals(false);
    toStateCombo->blockSignals(false);
}
