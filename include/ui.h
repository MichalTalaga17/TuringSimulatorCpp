#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QTimer>
#include <memory>
#include <map>
#include <set>
#include <vector>

#include "TuringMachine.h"

/// Main window for the Turing Machine Simulator with modern Windows 11 UI style
class TuringSimulatorUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit TuringSimulatorUI(QWidget *parent = nullptr);
    ~TuringSimulatorUI() = default;

protected:
    /// Reaguje na zmianę rozmiaru okna i ponownie centruje aktywną komórkę taśmy.
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Builder slots
    void onNumStatesChanged(int value);
    void onAlphabetChanged();
    void onBlankSymbolChanged();
    void onCellComboChanged();
    void onTapeInputChanged();

    // Execution slots
    void onStepClicked();
    void onRunClicked();
    void onResetClicked();
    void onSpeedChanged(int value);
    void onExecutionTimer();

    // File I/O slots
    void onSaveToFileClicked();
    void onLoadFromFileClicked();

private:
    /// Buduje główny układ okna jako jedną wspólną stronę aplikacji.
    void setupUI();
    /// Tworzy edytor logiki z konfiguracją maszyny i tabelą przejść.
    QWidget *setupBuilderTab();
    /// Tworzy panel uruchamiania symulacji oraz widok taśmy.
    QWidget *setupExecutorTab();
    /// Nakłada wspólny styl wizualny na całą aplikację.
    void applyModernStyle();
    /// Odświeża wszystkie elementy prezentujące bieżący stan maszyny.
    void updateDisplay();
    /// Aktualizuje etykiety stanu sterowania i licznika kroków.
    void updateStateDisplay();
    /// Odtwarza aktualny wycinek taśmy i centruje aktywną komórkę.
    void updateTapeDisplay();
    /// Planuje pojedyncze odświeżenie widoku taśmy po zakończeniu bieżącej pętli zdarzeń.
    void scheduleTapeDisplayRefresh();
    /// Tworzy zestaw widocznych komórek używanych do wizualizacji taśmy.
    void setupTapeCells();
    /// Wypełnia formularz i tabelę przykładową maszyną startową.
    void applyDefaultExample();
    /// Przebudowuje maszynę z edytora i wczytuje aktualną zawartość pola taśmy.
    bool rebuildMachineFromEditor();
    /// Buduje maszynę z aktualnej konfiguracji formularza i opcjonalnie pokazuje komunikat sukcesu.
    bool buildMachineFromCurrentConfiguration(bool showSuccessDialog);
    /// Tworzy pojedynczą komórkę tabeli przejść z kontrolkami zapisu, ruchu i stanu docelowego.
    QWidget *createTransitionCell(int row, int col);
    /// Przebudowuje tabelę przejść po zmianie liczby stanów lub alfabetu.
    void rebuildTransitionsTable();
    /// Czyści pomocniczą pamięć podręczną wskaźników do kontrolek tabeli.
    void clearTableCache();
    /// Ustawia domyślne wartości dla pojedynczej komórki tabeli przejść.
    void setCellDefaults(int row, int col, QComboBox *writeCombo, QComboBox *moveCombo, QComboBox *toStateCombo);
    /// Synchronizuje mapę przejść z aktualną zawartością tabeli.
    void populateTransitionMapWithDefaults();

    // Builder Components
    QSpinBox *numStatesSpinBox;
    QLineEdit *alphabetInputField;
    QLineEdit *blankSymbolField;
    QComboBox *initialStateCombo;
    QTableWidget *transitionsTable;
    QPushButton *saveToFileButton;
    QPushButton *loadFromFileButton;

    // Executor Components
    QLabel *machineStateLabel;
    QLabel *stepCountLabel;
    /// Przewijany kontener widoku taśmy używany do centrowania aktywnej komórki.
    QScrollArea *tapeScrollArea;
    /// Kontener z widocznymi komórkami reprezentującymi wycinek taśmy.
    QWidget *tapeDisplay;
    QLineEdit *tapeInputField;
    QPushButton *stepButton;
    QPushButton *runButton;
    QPushButton *resetButton;
    QSlider *speedSlider;
    QLabel *speedLabel;
    QLabel *statusLabel;

    // Logic
    std::unique_ptr<TuringMachine> machine;
    std::vector<std::string> states; // q0, q1, q2, ...
    std::set<char> alphabet;
    std::string initialState;
    std::set<std::string> acceptingStates;

    // Maps: (state, readSymbol) -> (writeSymbol, moveString, nextState)
    std::map<std::pair<std::string, char>, std::tuple<char, std::string, std::string>> transitionMap;

    char blankSymbol;

    bool isRunning;
    int executionSpeed;
    std::unique_ptr<QTimer> executionTimer;
    /// Jednostrzałowy timer współdzielony przez odświeżenia widoku taśmy, aby uniknąć kolejkowania duplikatów.
    std::unique_ptr<QTimer> tapeDisplayRefreshTimer;
    /// Bufor etykiet pojedynczych komórek taśmy używanych przy odświeżaniu widoku.
    std::vector<QLabel *> tapeCells;

    // Cache for combo boxes to speed up lookup
    std::map<std::pair<int, int>, std::tuple<QComboBox *, QComboBox *, QComboBox *>> comboCache;
};