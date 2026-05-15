#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QSlider>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTimer>
#include <memory>
#include <map>
#include <set>
#include <vector>

#include "TuringMachine.h"

/// Main window for the Turing Machine Simulator with modern Windows 11 UI style
class TuringSimulatorUI : public QMainWindow {
    Q_OBJECT

public:
    explicit TuringSimulatorUI(QWidget *parent = nullptr);
    ~TuringSimulatorUI() = default;

private slots:
    // Builder slots
    void onNumStatesChanged(int value);
    void onAlphabetChanged();
    void onBlankSymbolChanged();
    void onCellComboChanged();
    void onBuildMachineClicked();

    // Execution slots
    void onStepClicked();
    void onRunClicked();
    void onResetClicked();
    void onSpeedChanged(int value);
    void onExecutionTimer();

    // Input slots
    void onLoadTapeClicked();

private:
    void setupUI();
    QWidget* setupBuilderTab();
    QWidget* setupExecutorTab();
    void applyModernStyle();
    void updateDisplay();
    void updateStateDisplay();
    void updateTapeDisplay();
    void validateAndBuildMachine();
    void rebuildTransitionsTable();
    void clearTableCache();
    void setCellDefaults(int row, int col, QComboBox *writeCombo, QComboBox *moveCombo, QComboBox *toStateCombo);
    void populateTransitionMapWithDefaults();

    // Builder Components
    QSpinBox *numStatesSpinBox;
    QLineEdit *alphabetInputField;
    QLineEdit *blankSymbolField;
    QComboBox *initialStateCombo;
    QTableWidget *transitionsTable;
    QPushButton *buildMachineButton;

    // Executor Components
    QLabel *machineStateLabel;
    QLabel *stepCountLabel;
    QTextEdit *tapeDisplay;
    QLineEdit *tapeInputField;
    QPushButton *loadTapeButton;
    QPushButton *stepButton;
    QPushButton *runButton;
    QPushButton *resetButton;
    QSlider *speedSlider;
    QLabel *speedLabel;
    QLabel *statusLabel;

    // Logic
    std::unique_ptr<TuringMachine> machine;
    std::vector<std::string> states;  // q0, q1, q2, ...
    std::set<char> alphabet;
    std::string initialState;
    std::set<std::string> acceptingStates;

    // Maps: (state, readSymbol) -> (writeSymbol, moveString, nextState)
    std::map<std::pair<std::string, char>, std::tuple<char, std::string, std::string>> transitionMap;

    char blankSymbol;

    bool isRunning;
    int executionSpeed;
    std::unique_ptr<QTimer> executionTimer;

    // Cache for combo boxes to speed up lookup
    std::map<std::pair<int, int>, std::tuple<QComboBox*, QComboBox*, QComboBox*>> comboCache;
};
