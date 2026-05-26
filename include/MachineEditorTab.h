#pragma once

/**
 * @file MachineEditorTab.h
 * @brief Karta edycji, walidacji, zapisu i uruchamiania pojedynczej maszyny.
 */

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
#include <tuple>
#include <vector>
#include "storage.h"
#include "TuringMachine.h"

class QGroupBox;

/**
 * @brief Pojedyncza karta edycji i uruchamiania maszyny Turinga.
 *
 * Klasa łączy formularz definicji maszyny, tabelę przejść, widok taśmy,
 * sterowanie symulacją oraz operacje zapisu/odczytu. Logika została
 * rozdzielona na kilka plików `.cpp`, ale wszystkie metody należą do tej
 * jednej klasy widżetu.
 */
class MachineEditorTab : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Tworzy nową kartę edytora.
     * @param untitledTitle Tytuł używany dopóki karta nie ma pliku ani nazwy z bazy.
     * @param startWithExample Czy wypełnić kartę przykładową maszyną.
     * @param parent Opcjonalny rodzic Qt.
     */
    explicit MachineEditorTab(const QString &untitledTitle, bool startWithExample = true, QWidget *parent = nullptr);
    ~MachineEditorTab() = default;

    /**
     * @brief Zwraca tytuł prezentowany na karcie.
     * @return Nazwa pliku, nazwa z bazy lub tytuł tymczasowy.
     */
    QString tabTitle() const;
    /**
     * @brief Wczytuje maszynę ze wskazanego pliku do tej karty.
     * @param filePath Ścieżka pliku JSON.
     * @return `true`, jeśli dane zostały poprawnie odczytane i zastosowane.
     */
    bool loadFromFile(const QString &filePath);
    /**
     * @brief Zapisuje maszynę do znanej ścieżki albo pyta o nią, jeśli karta jest nowa.
     * @return `true`, jeśli zapis zakończył się powodzeniem.
     */
    bool saveToFile();
    /**
     * @brief Zapisuje maszynę po wskazaniu ścieżki przez użytkownika.
     * @return `true`, jeśli użytkownik wybrał plik i zapis się powiódł.
     */
    bool saveToFileAs();
    /**
     * @brief Przebudowuje maszynę i pokazuje wynik walidacji.
     * @return `true`, jeśli aktualna konfiguracja jest poprawna.
     */
    bool validateMachine();
    /**
     * @brief Przywraca przykład startowy w bieżącej karcie.
     */
    void resetToDefaultExample();
    /**
     * @brief Zapisuje aktualną maszynę do lokalnej bazy SQLite.
     * @return `true`, jeśli zapis do bazy się powiódł.
     */
    bool saveToDatabase();
    /**
     * @brief Wczytuje maszynę z lokalnej bazy SQLite.
     * @param machineName Nazwa maszyny w bazie.
     * @return `true`, jeśli rekord został znaleziony i odtworzony w UI.
     */
    bool loadFromDatabase(const QString &machineName);
    /**
     * @brief Pyta o opis zadania i generuje maszynę przez Gemini API.
     */
    void generateMachineWithAi();
    /**
     * @brief Generuje maszynę przez Gemini API z gotową konfiguracją połączenia.
     */
    void generateMachineWithAi(
        const QString &description,
        const QString &apiKey,
        const QString &modelName,
        const QString &fallbackModelName,
        bool thinkingEnabled);
    /**
     * @brief Wczytuje maszynę wygenerowaną poza kartą edytora.
     */
    void applyGeneratedMachineData(const MachineData &data);
    /**
     * @brief Odświeża teksty interfejsu po zmianie języka.
     */
    void retranslateUi();
    /**
     * @brief Pyta o opis maszyny, model i klucz API potrzebne do generowania.
     */
    static bool resolveGeminiGenerationRequest(
        QWidget *parent,
        QString *description,
        QString *apiKey,
        QString *modelName,
        QString *fallbackModelName,
        bool *thinkingEnabled);

signals:
    /**
     * @brief Emitowany, gdy tytuł karty powinien zostać odświeżony w QTabWidget.
     * @param title Nowy tytuł karty.
     */
    void titleChanged(const QString &title);

public slots:
    /**
     * @brief Obsługuje wykonanie jednego kroku symulacji.
     */
    void onStepClicked();
    /**
     * @brief Obsługuje uruchomienie lub pauzę automatycznej symulacji.
     */
    void onRunClicked();
    /**
     * @brief Resetuje maszynę do stanu początkowego i wejściowej taśmy.
     */
    void onResetClicked();

protected:
    /**
     * @brief Reaguje na zmianę rozmiaru okna i ponownie centruje aktywną komórkę taśmy.
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
     * @brief Aktualizuje listę stanów po zmianie liczby stanów.
     */
    void onNumStatesChanged(int value);
    /**
     * @brief Aktualizuje alfabet i tabelę przejść po zmianie pola alfabetu.
     */
    void onAlphabetChanged();
    /**
     * @brief Zmienia symbol pusty i przebudowuje alfabet.
     */
    void onBlankSymbolChanged();
    /**
     * @brief Synchronizuje mapę przejść po ręcznej edycji komórki tabeli.
     */
    void onTableItemChanged(QTableWidgetItem *item);
    /**
     * @brief Reaguje na zmianę wejściowej taśmy.
     */
    void onTapeInputChanged();

    /**
     * @brief Aktualizuje prędkość automatycznego wykonywania.
     */
    void onSpeedChanged(int value);
    /**
     * @brief Wykonuje kolejny krok wywołany przez timer symulacji.
     */
    void onExecutionTimer();

private:
    /// Buduje główny układ okna jako jedną wspólną stronę aplikacji.
    void setupUI();
    /// Tworzy edytor logiki z konfiguracją maszyny i tabelą przejść.
    QWidget *setupBuilderTab();
    /// Tworzy panel uruchamiania symulacji oraz widok taśmy.
    QWidget *setupExecutorTab();
    /// Odświeża wszystkie elementy prezentujące bieżący stan maszyny.
    void updateDisplay();
    /// Podświetla komórkę tabeli przejść odpowiadająca aktualnie wykonywanej instrukcji.
    void updateActiveTransitionHighlight();
    /// Podświetla wskazaną instrukcję przejścia.
    void highlightTransition(const std::string &state, char readSymbol);
    /// Czyści podświetlenie komórki przejścia.
    void clearActiveTransitionHighlight();
    /// Sprawdza, czy taśma zawiera niezdefiniowane symbole.
    void checkTapeForUndefinedSymbols();
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
    /// Wykonuje jeden krok i spójnie kończy/pauzuje UI, jeśli maszyna się zatrzyma.
    bool executeMachineStep();
    /// Buduje maszynę z aktualnej konfiguracji formularza i opcjonalnie pokazuje komunikat sukcesu.
    bool buildMachineFromCurrentConfiguration(bool showSuccessDialog);
    /// Przebudowuje tabelę przejść po zmianie liczby stanów lub alfabetu.
    void rebuildTransitionsTable();
    /// Synchronizuje mapę przejść z aktualną zawartością tabeli.
    void populateTransitionMapWithDefaults();
    /// Zapisuje aktualną definicję maszyny do wskazanej ścieżki.
    bool saveMachineToPath(const QString &filePath);
    /// Stosuje dane maszyny odczytane z pliku lub bazy do formularza i tabeli.
    void applyMachineData(const MachineData& data);
    /// Ustawia status wykonania według klucza tłumaczenia.
    void setStatusText(const char *key);
    /// Ustawia status wykonania według klucza tłumaczenia i argumentu.
    void setStatusText(const char *key, const QString &argument);
    /// Odświeża etykietę statusu z zapamiętanego klucza.
    void updateStatusLabelText();
    /// Tłumaczy kierunki ruchu widoczne w tabeli przejść.
    void retranslateTransitionMoves();

    /// @name Komponenty edytora definicji
    /// @{
    QSpinBox *numStatesSpinBox;
    QLineEdit *alphabetInputField;
    QLineEdit *blankSymbolField;
    QComboBox *initialStateCombo;
    QPushButton *aiGenerateButton;
    QTableWidget *transitionsTable;
    QGroupBox *logicEditorGroup;
    QLabel *statesLabel;
    QLabel *alphabetLabel;
    QLabel *blankLabel;
    QLabel *initialLabel;
    /// @}

    /// @name Komponenty panelu wykonania
    /// @{
    QLabel *machineStateLabel;
    QLabel *stepCountLabel;
    QLabel *stepMetricTitleLabel;
    QLabel *stateMetricTitleLabel;
    QLabel *modeMetricTitleLabel;
    /// Przewijany kontener widoku taśmy używany do centrowania aktywnej komórki.
    QScrollArea *tapeScrollArea;
    /// Kontener z widocznymi komórkami reprezentującymi wycinek taśmy.
    QWidget *tapeDisplay;
    QLineEdit *tapeInputField;
    QLabel *tapeWarningLabel;
    QPushButton *stepButton;
    QPushButton *runButton;
    QPushButton *resetButton;
    QSlider *speedSlider;
    QLabel *speedLabel;
    QLabel *statusLabel;
    QLabel *tapeInputLabel;
    QGroupBox *statusGroup;
    QGroupBox *inputGroup;
    QGroupBox *tapeGroup;
    /// @}

    /// @name Stan logiczny maszyny i symulacji
    /// @{
    std::unique_ptr<TuringMachine> machine;
    /// Lista stanów generowanych jako `q0`, `q1`, ...
    std::vector<std::string> states;
    std::vector<char> alphabet;
    std::string initialState;

    /// Mapa przejść: `(stan, symbol odczytu) -> (symbol zapisu, ruch, następny stan)`.
    std::map<std::pair<std::string, char>, std::tuple<char, std::string, std::string>> transitionMap;

    char blankSymbol;

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
    bool aiGenerationInProgress;
    QString currentStatusKey;
    QString currentStatusArgument;
    /// @}
public:
    /**
     * @brief Zwraca alfabet z symbolem pustym zawsze na pierwszej pozycji.
     * @return Uporządkowany alfabet używany przez tabelę przejść.
     */
    std::vector<char> getOrderedAlphabet() const;
    /**
     * @brief Udostępnia listę stanów delegate'owi tabeli przejść.
     * @return Niemodyfikowalna lista nazw stanów.
     */
    const std::vector<std::string>& getStatesList() const { return states; }
};
