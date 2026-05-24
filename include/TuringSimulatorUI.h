#pragma once

/**
 * @file TuringSimulatorUI.h
 * @brief Główne okno aplikacji z menu, kartami i stroną startową.
 */

#include <QAction>
#include <QMainWindow>
#include <QTabWidget>
#include <memory>

#include "MachineEditorTab.h"
#include "ThemeManager.h"

class QPushButton;
class QResizeEvent;

/**
 * @brief Główne okno symulatora z kartami dokumentów i paskiem menu.
 *
 * Odpowiada za tworzenie kart MachineEditorTab, stronę startową, akcje menu,
 * przełączanie motywu oraz operacje wysokiego poziomu delegowane do bieżącej
 * karty maszyny.
 */
class TuringSimulatorUI : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Tworzy główne okno aplikacji i inicjalizuje menu, karty oraz motyw.
     * @param parent Opcjonalny rodzic Qt.
     */
    explicit TuringSimulatorUI(QWidget *parent = nullptr);
    ~TuringSimulatorUI() = default;

protected:
    /**
     * @brief Aktualizuje elementy zależne od rozmiaru okna.
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /// Pokazuje lub tworzy stronę startową.
    void showStartPage();
    /// Tworzy nową pustą kartę maszyny.
    void createNewMachineTab();
    /// Tworzy nową kartę z przykładową maszyną.
    void createExampleMachineTab();
    /// Otwiera maszynę z pliku JSON w nowej karcie.
    void openMachineInNewTab();
    /// Zapisuje bieżącą kartę do znanej ścieżki albo uruchamia "Zapisz jako".
    void saveCurrentMachine();
    /// Zapisuje bieżącą kartę po wskazaniu ścieżki.
    void saveCurrentMachineAs();
    /// Pokazuje dialog wyboru maszyny z bazy danych.
    void openMachineFromDatabase();
    /// Zapisuje bieżącą maszynę w bazie danych.
    void saveCurrentMachineToDatabase();
    /// Zamyka aktualnie aktywną kartę.
    void closeCurrentMachineTab();
    /// Waliduje i przebudowuje bieżącą maszynę.
    void validateCurrentMachine();
    /// Przywraca przykład startowy w bieżącej karcie.
    void resetCurrentMachineToExample();
    /// Wykonuje pojedynczy krok bieżącej maszyny.
    void stepCurrentMachine();
    /// Przełącza tryb run/pause bieżącej maszyny.
    void toggleCurrentMachineRun();
    /// Resetuje bieżącą maszynę.
    void resetCurrentMachine();
    /// Aktywuje poprzednią kartę.
    void selectPreviousTab();
    /// Aktywuje następną kartę.
    void selectNextTab();
    /// Odświeża dostępność akcji menu i tytuł okna.
    void updateMenuState();
    /// Pokazuje okno informacji o programie.
    void showAboutDialog();
    /// Pokazuje okno informacji o autorach.
    void showAboutAuthorsDialog();
    /// Przełącza jasny i ciemny motyw aplikacji.
    void toggleTheme();

private:
    /// Buduje główny QTabWidget i przycisk dodawania kart.
    void setupUI();
    /// Tworzy menu aplikacji i podpina skróty klawiaturowe.
    void setupMenu();
    /// Zwraca aktualną kartę maszyny albo `nullptr`, jeśli aktywna jest strona startowa.
    MachineEditorTab *currentMachineTab() const;
    /// Tworzy widżet strony startowej.
    QWidget *createStartPage();
    /// Sprawdza, czy widżet jest stroną startową.
    bool isStartPage(QWidget *widget) const;
    /// Dodaje kartę z własnym przyciskiem zamykania.
    int addClosableTab(QWidget *tab, const QString &title, int insertIndex = -1);
    /// Dodaje kartę maszyny i podłącza sygnał zmiany tytułu.
    void addMachineTab(MachineEditorTab *tab, int insertIndex = -1);
    /// Dodaje kartę maszyny, zastępując stronę startową, jeśli była aktywna.
    void addMachineTabFromStartPage(MachineEditorTab *tab);
    /// Otwiera wskazaną maszynę z bazy w nowej karcie.
    void openMachineFromDatabaseName(const QString &machineName);
    /// Zamyka kartę o podanym indeksie.
    void closeTabAt(int index);
    /// Utrzymuje przycisk nowej karty nad paskiem kart.
    void updateNewTabButtonPosition();

    /// Kontener kart maszyn i strony startowej.
    QTabWidget *machineTabs;
    /// Przycisk `+` tworzący lub pokazujący stronę startową.
    QPushButton *newTabButton;
    /// @name Akcje menu zależne od aktywnej karty
    /// @{
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *closeTabAction;
    QAction *validateAction;
    QAction *resetExampleAction;
    QAction *stepAction;
    QAction *runAction;
    QAction *resetAction;
    QAction *previousTabAction;
    QAction *nextTabAction;
    QAction *toggleThemeAction;
    /// @}
    /// Licznik tytułów nowych, niezapisanych kart.
    int untitledTabCounter;
    /// Manager globalnego motywu aplikacji.
    std::unique_ptr<ThemeManager> themeManager;
};
