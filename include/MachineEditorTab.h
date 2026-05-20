#pragma once

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
#include "storage.h"
#include "TuringMachine.h"

/// Pojedyncza karta edycji i uruchamiania maszyny Turinga.
class MachineEditorTab : public QWidget
{
    Q_OBJECT

public:
    explicit MachineEditorTab(const QString &untitledTitle, bool startWithExample = true, QWidget *parent = nullptr);
    ~MachineEditorTab() = default;

    /// Zwraca tytuł prezentowany na karcie.
    QString tabTitle() const;
    /// Wczytuje maszynę ze wskazanego pliku do tej karty.
    bool loadFromFile(const QString &filePath);
    /// Zapisuje maszynę do znanej ścieżki albo pyta o nią, jeśli karta jest nowa.
    bool saveToFile();
    /// Zapisuje maszynę po wskazaniu ścieżki przez użytkownika.
    bool saveToFileAs();
    /// Przebudowuje maszynę i pokazuje wynik walidacji.
    bool validateMachine();
    /// Przywraca przykład startowy w bieżącej karcie.
    void resetToDefaultExample();
    /// Zapisuje do bazy danych.
    bool saveToDatabase();
    /// Wczytuje z bazy danych.
    bool loadFromDatabase(const QString &machineName);

signals:
    void titleChanged(const QString &title);

public slots:
    // Execution slots
    void onStepClicked();
    void onRunClicked();
    void onResetClicked();

protected:
    /// Reaguje na zmianę rozmiaru okna i ponownie centruje aktywną komórkę taśmy.
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Builder slots
    void onNumStatesChanged(int value);
    void onAlphabetChanged();
    void onBlankSymbolChanged();
    void onTableItemChanged(QTableWidgetItem *item);
    void onTapeInputChanged();

    void onSpeedChanged(int value);
    void onExecutionTimer();

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
    /// Podświetla komórkę tabeli przejść odpowiadająca aktualnie wykonywanej instrukcji.
    void updateActiveTransitionHighlight();
    /// Czyści podświetlenie komórki przejścia.
    void clearActiveTransitionHighlight();
    /// Ustawia wizualny stan pojedynczej komórki tabeli przejść.
    void setTransitionCellHighlighted(int row, int col, bool highlighted);
    /// Aktualizuje etykiety stanu sterowania i licznika kroków.
    void updateStateDisplay();
    /// Odtwarza aktualny wycinek taśmy i centruje aktywną komórkę.
    void updateTapeDisplay();
    /// Planuje pojedyncze odświeżenie widoku taśmy po zakończeniu bieżącej pętli zdarzeń.
    void scheduleTapeDisplayRefresh();
    /// Tworzy zestaw widocznych komórek używanych do wizualizacji taśmy.
    void setupTapeCells();
    /// Rozszerza bufor komórek, gdy wpisana taśma wejściowa nie mieści się w wizualizacji.
    void ensureTapeCellsFitInput();
    /// Tworzy pojedynczą etykietę komórki taśmy.
    QLabel *createTapeCell();
    /// Nakłada styl zwykłej albo aktywnej komórki taśmy.
    void applyTapeCellStyle(QLabel *cell, bool active);
    /// Wypełnia formularz i tabelę przykładową maszyną startową.
    void applyDefaultExample();
    /// Wypełnia formularz minimalną pustą konfiguracją do pracy od zera.
    void applyBlankMachine();
    /// Przebudowuje maszynę z edytora i wczytuje aktualną zawartość pola taśmy.
    bool rebuildMachineFromEditor();
    /// Buduje maszynę z aktualnej konfiguracji formularza i opcjonalnie pokazuje komunikat sukcesu.
    bool buildMachineFromCurrentConfiguration(bool showSuccessDialog);
    /// Przebudowuje tabelę przejść po zmianie liczby stanów lub alfabetu.
    void rebuildTransitionsTable();
    /// Zmienia symbol w kolumnie bez przebudowania całej tabeli (dla zmiany symbolu pustego).
    void changeColumnSymbol(int col, char oldSymbol, char newSymbol);
    /// Synchronizuje mapę przejść z aktualną zawartością tabeli.
    void populateTransitionMapWithDefaults();
    /// Zapisuje aktualną definicję maszyny do wskazanej ścieżki.
    bool saveMachineToPath(const QString &filePath);
    void applyMachineData(const MachineData& data);

    // Builder Components
    QSpinBox *numStatesSpinBox;
    QLineEdit *alphabetInputField;
    QLineEdit *blankSymbolField;
    QComboBox *initialStateCombo;
    QTableWidget *transitionsTable;

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
    char previousBlankSymbol;

    bool isRunning;
    int executionSpeed;
    std::unique_ptr<QTimer> executionTimer;
    /// Jednostrzałowy timer współdzielony przez odświeżenia widoku taśmy, aby uniknąć kolejkowania duplikatów.
    std::unique_ptr<QTimer> tapeDisplayRefreshTimer;
    /// Bufor etykiet pojedynczych komórek taśmy używanych przy odświeżaniu widoku.
    std::vector<QLabel *> tapeCells;

    QString untitledTitle;
    QString currentFilePath;
    bool startWithExample;
    int highlightedTransitionRow;
    int highlightedTransitionCol;
public:
    std::vector<char> getOrderedAlphabet() const;
    const std::vector<std::string>& getStatesList() const { return states; }
};
