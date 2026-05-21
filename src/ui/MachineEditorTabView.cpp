#include "MachineEditorTab.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QListView>
#include <QPalette>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QStyleFactory>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QRegularExpressionValidator>

#include <tuple>

namespace
{
    class TransitionEditor : public QWidget
    {
        Q_OBJECT
    public:
        QComboBox *writeCombo;
        QComboBox *moveCombo;
        QComboBox *stateCombo;

        TransitionEditor(QWidget *parent = nullptr) : QWidget(parent)
        {
            QHBoxLayout *l = new QHBoxLayout(this);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(2);

            writeCombo = new QComboBox(this);
            writeCombo->setFixedHeight(26);
            writeCombo->setProperty("isCompact", true);
            writeCombo->setMaxVisibleItems(8);
            auto wv = new QListView(this);
            wv->setMinimumWidth(50);
            writeCombo->setView(wv);

            moveCombo = new QComboBox(this);
            moveCombo->setFixedHeight(26);
            moveCombo->setProperty("isCompact", true);
            moveCombo->setMaxVisibleItems(8);
            auto mv = new QListView(this);
            mv->setMinimumWidth(65);
            moveCombo->setView(mv);

            stateCombo = new QComboBox(this);
            stateCombo->setFixedHeight(26);
            stateCombo->setProperty("isCompact", true);
            stateCombo->setMaxVisibleItems(8);
            auto sv = new QListView(this);
            sv->setMinimumWidth(70);
            stateCombo->setView(sv);

            l->addWidget(writeCombo);
            l->addWidget(moveCombo);
            l->addWidget(stateCombo);
            setFocusPolicy(Qt::StrongFocus);
            setFocusProxy(writeCombo);
            setAutoFillBackground(true);
        }
    };

    class TransitionDelegate : public QStyledItemDelegate
    {
    private:
        MachineEditorTab *m_tab;

    public:
        TransitionDelegate(MachineEditorTab *tab, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_tab(tab) {}

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            TransitionEditor *editor = new TransitionEditor(parent);
            
            // Populate write combos
            for (char c : m_tab->getOrderedAlphabet()) {
                editor->writeCombo->addItem(QString(c));
            }
            
            // Populate move combos
            editor->moveCombo->addItems({"L", "R", "Stay", "Stop"});
            
            // Populate state combos
            for (const auto &s : m_tab->getStatesList()) {
                editor->stateCombo->addItem(QString::fromStdString(s));
            }

            auto commitChanges = [this, editor]() {
                auto self = const_cast<TransitionDelegate*>(this);
                emit self->commitData(editor);
            };

            connect(editor->writeCombo, &QComboBox::currentTextChanged, editor, commitChanges);
            connect(editor->moveCombo, &QComboBox::currentTextChanged, editor, commitChanges);
            connect(editor->stateCombo, &QComboBox::currentTextChanged, editor, commitChanges);

            return editor;
        }

        void setEditorData(QWidget *widget, const QModelIndex &index) const override
        {
            TransitionEditor *editor = qobject_cast<TransitionEditor *>(widget);
            if (!editor) return;

            QString text = index.data(Qt::DisplayRole).toString();
            QStringList parts = text.split(",");
            for (QString &part : parts) {
                part = part.trimmed();
            }
            if (parts.size() == 3) {
                editor->writeCombo->setCurrentText(parts[0]);
                editor->moveCombo->setCurrentText(parts[1]);
                editor->stateCombo->setCurrentText(parts[2]);
            }
        }

        void setModelData(QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const override
        {
            TransitionEditor *editor = qobject_cast<TransitionEditor *>(widget);
            if (!editor) return;

            QString text = QString("%1, %2, %3")
                               .arg(editor->writeCombo->currentText())
                               .arg(editor->moveCombo->currentText())
                               .arg(editor->stateCombo->currentText());
                               
            model->setData(index, text, Qt::DisplayRole);
        }

        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            editor->setGeometry(option.rect);
        }
    };
} // namespace

MachineEditorTab::MachineEditorTab(const QString &untitledTitle, bool startWithExample, QWidget *parent)
    : QWidget(parent), numStatesSpinBox(nullptr), alphabetInputField(nullptr), blankSymbolField(nullptr), initialStateCombo(nullptr), transitionsTable(nullptr), machineStateLabel(nullptr), stepCountLabel(nullptr), tapeScrollArea(nullptr), tapeDisplay(nullptr), tapeInputField(nullptr), stepButton(nullptr), runButton(nullptr), resetButton(nullptr), speedSlider(nullptr), speedLabel(nullptr), statusLabel(nullptr), machine(nullptr), blankSymbol('_'), previousBlankSymbol('_'), isRunning(false), executionSpeed(500), executionTimer(std::make_unique<QTimer>()), tapeDisplayRefreshTimer(std::make_unique<QTimer>()), untitledTitle(untitledTitle), startWithExample(startWithExample), highlightedTransitionRow(-1), highlightedTransitionCol(-1)
{
    setupUI();

    // Connect timer for execution
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

    // --- Executor Panel FIRST (so statusLabel is created early) ---
    pageLayout->addWidget(setupExecutorTab(), 0);

    // --- Builder Panel SECOND (can safely use statusLabel) ---
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
    // Restrict to standard printable ASCII characters (32 to 126)
    alphabetInputField->setValidator(new QRegularExpressionValidator(QRegularExpression("[ -~]*"), this));
    alphabetInputField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    configFieldsLayout->addWidget(alphabetInputField, 1);

    configFieldsLayout->addWidget(new QLabel("Blank:"));
    blankSymbolField = new QLineEdit();
    blankSymbolField->setText("_");
    // Restrict to one standard printable ASCII character, excluding space
    blankSymbolField->setValidator(new QRegularExpressionValidator(QRegularExpression("[!-~]"), this));
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

    // Connect signals (after all widgets are created)
    connect(numStatesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MachineEditorTab::onNumStatesChanged);
    connect(alphabetInputField, &QLineEdit::textChanged, this, &MachineEditorTab::onAlphabetChanged);
    connect(blankSymbolField, &QLineEdit::textChanged, this, &MachineEditorTab::onBlankSymbolChanged);
    connect(initialStateCombo, &QComboBox::currentTextChanged, this, [this](const QString &state)
            {
        initialState = state.toStdString();
        if (rebuildMachineFromEditor() && statusLabel) {
            statusLabel->setText("Rebuilt");
        } });
    
    connect(transitionsTable, &QTableWidget::itemChanged, this, &MachineEditorTab::onTableItemChanged);

    // Initialize with safe blocking
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

    // --- Status Panel ---
    QGroupBox *statusGroup = new QGroupBox("Machine Status", executorWidget);
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

    // Don't set hardcoded styles - use theme defaults
    // Theme stylesheets handle all button styling

    speedLabel = new QLabel("Speed:");
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

    // --- Tape Input Section ---
    QGroupBox *inputGroup = new QGroupBox("Input Tape", executorWidget);
    QVBoxLayout *inputGroupLayout = new QVBoxLayout(inputGroup);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Tape:"));
    tapeInputField = new QLineEdit();
    tapeInputField->setValidator(new QRegularExpressionValidator(QRegularExpression("[ -~]*"), this));
    tapeInputField->setPlaceholderText("e.g., 1001");
    inputLayout->addWidget(tapeInputField);

    inputGroupLayout->addLayout(inputLayout);

    tapeWarningLabel = new QLabel("Tape contains undefined symbols! Proceeding will halt the machine.", inputGroup);
    tapeWarningLabel->setStyleSheet("color: red; font-weight: bold;");
    tapeWarningLabel->hide(); // hidden by default
    inputGroupLayout->addWidget(tapeWarningLabel);

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
    connect(tapeInputField, &QLineEdit::textChanged, this, &MachineEditorTab::onTapeInputChanged);
    connect(stepButton, &QPushButton::clicked, this, &MachineEditorTab::onStepClicked);
    connect(runButton, &QPushButton::clicked, this, &MachineEditorTab::onRunClicked);
    connect(resetButton, &QPushButton::clicked, this, &MachineEditorTab::onResetClicked);
    connect(speedSlider, QOverload<int>::of(&QSlider::valueChanged), this, &MachineEditorTab::onSpeedChanged);

    return executorWidget;
}

void MachineEditorTab::applyModernStyle()
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
                stop: 0 #edf3fb,
                stop: 0.45 #f1f5fa,
                stop: 1 #f3f3f3
            );
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

#include "MachineEditorTabView.moc"
