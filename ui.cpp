#include "ui.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QStyleFactory>
#include <QPalette>
#include <QTabWidget>
#include <QMessageBox>
#include <QScrollBar>
#include <QTimer>
#include <QSpinBox>
#include <algorithm>
#include <sstream>
#include <tuple>

TuringSimulatorUI::TuringSimulatorUI(QWidget *parent)
    : QMainWindow(parent)
    , numStatesSpinBox(nullptr)
    , alphabetInputField(nullptr)
    , blankSymbolField(nullptr)
    , initialStateCombo(nullptr)
    , transitionsTable(nullptr)
    , buildMachineButton(nullptr)
    , machineStateLabel(nullptr)
    , stepCountLabel(nullptr)
    , tapeScrollArea(nullptr)
    , tapeDisplay(nullptr)
    , tapeInputField(nullptr)
    , loadTapeButton(nullptr)
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
{
    setupUI();
    applyModernStyle();

    // Connect timer for execution
    connect(executionTimer.get(), &QTimer::timeout, this, &TuringSimulatorUI::onExecutionTimer);

    QTimer::singleShot(0, this, [this]() {
        updateTapeDisplay();
    });
}

void TuringSimulatorUI::setupUI()
{
    setWindowTitle("Turing Machine Simulator");
    setGeometry(100, 100, 1200, 700);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    QTabWidget *tabWidget = new QTabWidget(this);

    // --- Executor Tab FIRST (so statusLabel is created early) ---
    QWidget *executorTab = setupExecutorTab();
    tabWidget->addTab(executorTab, "Executor");

    // --- Builder Tab SECOND (can safely use statusLabel) ---
    QWidget *builderTab = setupBuilderTab();
    tabWidget->addTab(builderTab, "Machine Builder");

    mainLayout->addWidget(tabWidget);
    setCentralWidget(centralWidget);
}

QWidget* TuringSimulatorUI::setupBuilderTab()
{
    QWidget *builderWidget = new QWidget();
    QVBoxLayout *builderLayout = new QVBoxLayout(builderWidget);
    builderLayout->setSpacing(12);

    // --- Configuration Section ---
    QGroupBox *configGroup = new QGroupBox("Machine Configuration", builderWidget);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);

    // Number of states
    QHBoxLayout *numStatesLayout = new QHBoxLayout();
    numStatesLayout->addWidget(new QLabel("Number of States:"));
    numStatesSpinBox = new QSpinBox();
    numStatesSpinBox->setMinimum(1);
    numStatesSpinBox->setMaximum(20);
    numStatesSpinBox->setValue(2);
    numStatesLayout->addWidget(numStatesSpinBox);
    numStatesLayout->addStretch();
    configLayout->addLayout(numStatesLayout);

    // Alphabet input
    QHBoxLayout *alphabetLayout = new QHBoxLayout();
    alphabetLayout->addWidget(new QLabel("Alphabet (e.g., 01_):"));
    alphabetInputField = new QLineEdit();
    alphabetInputField->setPlaceholderText("0,1");
    alphabetLayout->addWidget(alphabetInputField);
    configLayout->addLayout(alphabetLayout);

    // Blank symbol
    QHBoxLayout *blankLayout = new QHBoxLayout();
    blankLayout->addWidget(new QLabel("Blank Symbol:"));
    blankSymbolField = new QLineEdit();
    blankSymbolField->setText("_");
    blankSymbolField->setMaximumWidth(60);
    blankLayout->addWidget(blankSymbolField);
    blankLayout->addStretch();
    configLayout->addLayout(blankLayout);

    // Initial state
    QHBoxLayout *initialLayout = new QHBoxLayout();
    initialLayout->addWidget(new QLabel("Initial State:"));
    initialStateCombo = new QComboBox();
    initialLayout->addWidget(initialStateCombo);
    initialLayout->addStretch();
    configLayout->addLayout(initialLayout);

    configLayout->addStretch();
    builderLayout->addWidget(configGroup);

    // --- Transitions Table Section ---
    QGroupBox *transitionsGroup = new QGroupBox("Transitions Matrix (States × Alphabet)", builderWidget);
    QVBoxLayout *transitionsLayout = new QVBoxLayout(transitionsGroup);

    QLabel *instructionLabel = new QLabel("Select and modify transitions below. Defaults: Write = Read symbol, Move = Left, State = Current state");
    instructionLabel->setStyleSheet("color: #666666; font-size: 10px;");
    transitionsLayout->addWidget(instructionLabel);

    transitionsTable = new QTableWidget();
    transitionsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    transitionsTable->setMinimumHeight(300);
    transitionsTable->setRowHeight(0, 45);  // Taller rows for the 3-field display
    transitionsLayout->addWidget(transitionsTable);

    builderLayout->addWidget(transitionsGroup);

    // --- Build Button ---
    QHBoxLayout *buildLayout = new QHBoxLayout();
    buildLayout->addStretch();
    buildMachineButton = new QPushButton("Build Machine");
    buildMachineButton->setStyleSheet("background-color: #107c10; min-width: 150px; min-height: 40px; font-weight: bold;");
    buildLayout->addWidget(buildMachineButton);
    builderLayout->addLayout(buildLayout);

    // Connect signals (after all widgets are created)
    connect(numStatesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TuringSimulatorUI::onNumStatesChanged);
    connect(alphabetInputField, &QLineEdit::textChanged, this, &TuringSimulatorUI::onAlphabetChanged);
    connect(blankSymbolField, &QLineEdit::textChanged, this, &TuringSimulatorUI::onBlankSymbolChanged);
    connect(buildMachineButton, &QPushButton::clicked, this, &TuringSimulatorUI::onBuildMachineClicked);

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
    QVBoxLayout *executorLayout = new QVBoxLayout(executorWidget);
    executorLayout->setSpacing(12);

    // --- Status Panel ---
    QGroupBox *statusGroup = new QGroupBox("Machine Status", executorWidget);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);

    machineStateLabel = new QLabel("State: (not built)");
    machineStateLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    statusLayout->addWidget(machineStateLabel);

    stepCountLabel = new QLabel("Steps: 0");
    stepCountLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    statusLayout->addWidget(stepCountLabel);

    statusLabel = new QLabel("Status: Ready");
    statusLabel->setStyleSheet("font-size: 11px; color: #0078d4;");
    statusLayout->addWidget(statusLabel);

    statusLayout->addStretch();
    executorLayout->addWidget(statusGroup);

    // --- Tape Input Section ---
    QGroupBox *inputGroup = new QGroupBox("Input Tape", executorWidget);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputGroup);
    inputLayout->addWidget(new QLabel("Tape:"));
    tapeInputField = new QLineEdit();
    tapeInputField->setPlaceholderText("e.g., 1001");
    inputLayout->addWidget(tapeInputField);
    loadTapeButton = new QPushButton("Load Tape");
    inputLayout->addWidget(loadTapeButton);
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

    // --- Control Buttons ---
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    stepButton = new QPushButton("Step");
    runButton = new QPushButton("Run");
    resetButton = new QPushButton("Reset");

    buttonLayout->addWidget(stepButton);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();

    executorLayout->addLayout(buttonLayout);

    // --- Speed Control ---
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLabel = new QLabel("Speed:");
    speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setMinimum(100);
    speedSlider->setMaximum(2000);
    speedSlider->setValue(500);
    speedSlider->setMaximumWidth(300);

    speedLayout->addWidget(speedLabel);
    speedLayout->addWidget(speedSlider);
    speedLayout->addStretch();

    executorLayout->addLayout(speedLayout);
    executorLayout->addStretch();

    // Connect signals
    connect(loadTapeButton, &QPushButton::clicked, this, &TuringSimulatorUI::onLoadTapeClicked);
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
        QLineEdit, QTextEdit, QComboBox {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 6px;
            background-color: white;
            color: #202020;
            font-size: 11px;
        }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
            border: 2px solid #0078d4;
        }
        QTableWidget {
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            background-color: white;
            gridline-color: #e0e0e0;
        }
        QTableWidget::item {
            padding: 4px;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: #0078d4;
            color: white;
        }
        QHeaderView::section {
            background-color: #f0f0f0;
            color: #202020;
            padding: 6px;
            border: none;
            border-right: 1px solid #d0d0d0;
            font-weight: bold;
        }
        QLabel {
            color: #202020;
        }
        QTabWidget::pane {
            border: 1px solid #d0d0d0;
        }
        QTabBar::tab {
            background-color: #e8e8e8;
            color: #202020;
            padding: 8px 20px;
            margin-right: 2px;
            border: none;
        }
        QTabBar::tab:selected {
            background-color: white;
            border-bottom: 3px solid #0078d4;
        }
        QSlider::groove:horizontal {
            border: 1px solid #d0d0d0;
            height: 8px;
            background: white;
            margin: 2px 0;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: #0078d4;
            border: none;
            width: 18px;
            margin: -5px 0;
            border-radius: 9px;
        }
        QSlider::handle:horizontal:hover {
            background: #106ebe;
        }
        QSlider::sub-page:horizontal {
            background: #0078d4;
            border-radius: 4px;
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
                    // Create container widget with 3 dropdowns for new row
                    QWidget *cellWidget = new QWidget();
                    QHBoxLayout *layout = new QHBoxLayout(cellWidget);
                    layout->setContentsMargins(1, 1, 1, 1);
                    layout->setSpacing(1);

                    // Write Symbol Combo
                    QComboBox *writeCombo = new QComboBox();
                    writeCombo->setObjectName(QString("write_%1_%2").arg(row).arg(col));
                    writeCombo->setMinimumHeight(28);
                    writeCombo->setMaximumHeight(28);
                    writeCombo->setMinimumWidth(45);
                    writeCombo->setMaximumWidth(55);
                    for (char c : alphabet) {
                        writeCombo->addItem(QString(c));
                    }
                    writeCombo->setStyleSheet(
                        "QComboBox { border: 1px solid #ccc; border-radius: 3px; padding: 2px; }"
                        "QComboBox::drop-down { border: none; width: 20px; }"
                    );
                    layout->addWidget(writeCombo);

                    // Move Direction Combo
                    QComboBox *moveCombo = new QComboBox();
                    moveCombo->setObjectName(QString("move_%1_%2").arg(row).arg(col));
                    moveCombo->setMinimumHeight(28);
                    moveCombo->setMaximumHeight(28);
                    moveCombo->setMinimumWidth(50);
                    moveCombo->setMaximumWidth(60);
                    moveCombo->addItems({"L", "R", "Stay", "Stop"});
                    moveCombo->setStyleSheet(
                        "QComboBox { border: 1px solid #ccc; border-radius: 3px; padding: 2px; }"
                        "QComboBox::drop-down { border: none; width: 20px; }"
                    );
                    layout->addWidget(moveCombo);

                    // To State Combo
                    QComboBox *toStateCombo = new QComboBox();
                    toStateCombo->setObjectName(QString("tostate_%1_%2").arg(row).arg(col));
                    toStateCombo->setMinimumHeight(28);
                    toStateCombo->setMaximumHeight(28);
                    toStateCombo->setMinimumWidth(50);
                    toStateCombo->setMaximumWidth(60);
                    for (const auto& state : states) {
                        toStateCombo->addItem(QString::fromStdString(state));
                    }
                    toStateCombo->setStyleSheet(
                        "QComboBox { border: 1px solid #ccc; border-radius: 3px; padding: 2px; }"
                        "QComboBox::drop-down { border: none; width: 20px; }"
                    );
                    layout->addWidget(toStateCombo);

                    cellWidget->setLayout(layout);

                    // Add to table
                    transitionsTable->setCellWidget(row, col, cellWidget);

                    // Cache the combo pointers
                    comboCache[{row, col}] = std::make_tuple(writeCombo, moveCombo, toStateCombo);

                    // Set default values: write=read, move=L, state=current state
                    setCellDefaults(row, col, writeCombo, moveCombo, toStateCombo);

                    // Connect signals
                    connect(writeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
                    connect(moveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
                    connect(toStateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
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

    if (statusLabel) {
        statusLabel->setText("States added. Now define alphabet and fill transitions.");
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

    if (statusLabel) {
        statusLabel->setText(QString("Alphabet updated with %1 symbols (including blank). Fill in transitions.").arg(static_cast<int>(alphabet.size())));
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
        transitionMap.clear();

        if (statusLabel) {
            statusLabel->setText(QString("Blank symbol updated. Alphabet now has %1 symbols.").arg(static_cast<int>(alphabet.size())));
        }
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
}

void TuringSimulatorUI::onBuildMachineClicked()
{
    validateAndBuildMachine();
}

bool TuringSimulatorUI::buildMachineFromCurrentConfiguration(bool showSuccessDialog)
{
    // Validation
    if (states.empty()) {
        QMessageBox::warning(this, "Validation Error", "Please set the number of states");
        return false;
    }

    if (alphabet.empty()) {
        QMessageBox::warning(this, "Validation Error", "Please define the alphabet");
        return false;
    }

    if (initialState.empty()) {
        QMessageBox::warning(this, "Validation Error", "Please select an initial state");
        return false;
    }

    if (transitionMap.empty()) {
        QMessageBox::warning(this, "Validation Error", "Please define at least one transition");
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

    // If the UI does not define accepting states explicitly, build the machine without them.
    acceptingStates.clear();

    for (const auto& state : acceptingStates) {
        machine->addAcceptingState(state);
    }

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

    statusLabel->setText("✓ Machine built successfully!");
    statusLabel->setStyleSheet("font-size: 11px; color: #107c10; font-weight: bold;");

    if (showSuccessDialog) {
        QMessageBox::information(this, "Success", "Machine built successfully! Switch to Executor tab to run.");
    }

    return true;
}

void TuringSimulatorUI::validateAndBuildMachine()
{
    buildMachineFromCurrentConfiguration(true);
}

// === Executor Slots ===

void TuringSimulatorUI::onLoadTapeClicked()
{
    if (!machine) {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    QString tapeStr = tapeInputField->text().trimmed();
    if (tapeStr.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a tape content");
        return;
    }

    machine->loadTape(tapeStr.toStdString());
    updateDisplay();
    statusLabel->setText("Tape loaded. Ready to execute.");
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
        statusLabel->setText(machine->isAccepted() ? "✓ Accepted" : "✗ Halted");
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
    loadTapeButton->setEnabled(!isRunning);

    if (isRunning) {
        executionTimer->start(2001 - executionSpeed);
    } else {
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
        loadTapeButton->setEnabled(true);
        executionTimer->stop();
        updateDisplay();
        statusLabel->setText("Machine reset.");
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
            loadTapeButton->setEnabled(true);
            executionTimer->stop();
            statusLabel->setText(machine->isAccepted() ? "✓ Accepted" : "✗ Halted");
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
        machineStateLabel->setText(QString::fromStdString("State: " + machine->currentState()));
        stepCountLabel->setText(QString::fromStdString("Steps: " + std::to_string(machine->stepCount())));
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
        QTimer::singleShot(0, this, [this]() {
            updateTapeDisplay();
        });
        return;
    }

    const int targetValue = activeCell->x() + (activeCell->width() / 2) - (viewportWidth / 2);
    tapeScrollArea->horizontalScrollBar()->setValue(std::max(0, targetValue));
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

    if (buildMachineFromCurrentConfiguration(false) && machine) {
        machine->loadTape(tapeInputField->text().trimmed().toStdString());
        updateDisplay();
        statusLabel->setText("Example machine loaded. Ready to execute.");
        statusLabel->setStyleSheet("font-size: 11px; color: #0078d4;");
    }
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
            // Create container widget with 3 dropdowns
            QWidget *cellWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(cellWidget);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);

            // Write Symbol Combo
            QComboBox *writeCombo = new QComboBox();
            writeCombo->setObjectName(QString("write_%1_%2").arg(row).arg(col));
            writeCombo->setMinimumHeight(28);
            writeCombo->setMaximumHeight(28);
            writeCombo->setMinimumWidth(45);
            writeCombo->setMaximumWidth(55);
            for (char c : alphabet) {
                writeCombo->addItem(QString(c));
            }
            writeCombo->setStyleSheet(
                "QComboBox { border: 1px solid #ccc; border-radius: 3px; padding: 2px; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
            );
            layout->addWidget(writeCombo);

            // Move Direction Combo
            QComboBox *moveCombo = new QComboBox();
            moveCombo->setObjectName(QString("move_%1_%2").arg(row).arg(col));
            moveCombo->setMinimumHeight(28);
            moveCombo->setMaximumHeight(28);
            moveCombo->setMinimumWidth(50);
            moveCombo->setMaximumWidth(60);
            moveCombo->addItems({"L", "R", "Stay", "Stop"});
            moveCombo->setStyleSheet(
                "QComboBox { border: 1px solid #ccc; border-radius: 3px; padding: 2px; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
            );
            layout->addWidget(moveCombo);

            // To State Combo
            QComboBox *toStateCombo = new QComboBox();
            toStateCombo->setObjectName(QString("tostate_%1_%2").arg(row).arg(col));
            toStateCombo->setMinimumHeight(28);
            toStateCombo->setMaximumHeight(28);
            toStateCombo->setMinimumWidth(50);
            toStateCombo->setMaximumWidth(60);
            for (const auto& state : states) {
                toStateCombo->addItem(QString::fromStdString(state));
            }
            toStateCombo->setStyleSheet(
                "QComboBox { border: 1px solid #ccc; border-radius: 3px; padding: 2px; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
            );
            layout->addWidget(toStateCombo);

            cellWidget->setLayout(layout);

            // Add to table
            transitionsTable->setCellWidget(row, col, cellWidget);

            // Cache the combo pointers
            comboCache[{row, col}] = std::make_tuple(writeCombo, moveCombo, toStateCombo);

            // Set default values: write=read, move=L, state=current state
            setCellDefaults(row, col, writeCombo, moveCombo, toStateCombo);

            // Connect signals
            connect(writeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
            connect(moveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
            connect(toStateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TuringSimulatorUI::onCellComboChanged);
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
