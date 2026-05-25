#ifndef STORAGE_H
#define STORAGE_H

/**
 * @file storage.h
 * @brief API zapisu i odczytu definicji maszyn z plików JSON oraz SQLite.
 */

#include <QStringList>
#include <QString>
#include <QByteArray>

#include <string>
#include <tuple>
#include <vector>

/**
 * @brief Deklaracja zapowiadająca rdzeń maszyny bez dołączania pełnego nagłówka.
 */
class TuringMachine;

/**
 * @brief Dane definicji maszyny odczytane z trwałego nośnika.
 *
 * Struktura jest niezależna od UI. Warstwa MachineEditorTab używa jej do
 * odtworzenia pól formularza, tabeli przejść i parametrów rdzenia maszyny.
 */
struct MachineData {
    /// Liczba stanów `q0...qN` odtwarzana w edytorze.
    int numStates = 0;
    /// Alfabet maszyny, zwykle z symbolem pustym na pierwszej pozycji.
    std::vector<char> alphabet;
    /// Lista przejść: stan, odczyt, następny stan, zapis, ruch.
    std::vector<std::tuple<std::string, char, std::string, char, std::string>> transitions;

    /// Nazwa stanu początkowego.
    std::string initialState;
    /// Symbol pustej komórki taśmy.
    char blankSymbol = '_';

    /// Flaga powodzenia operacji odczytu.
    bool success = false;
    /// Komunikat błędu prezentowany użytkownikowi po nieudanym odczycie.
    QString errorMessage = "";
};

/**
 * @brief Zapisuje definicję maszyny do pliku JSON.
 *
 * @param machine Zbudowana maszyna zawierająca przejścia.
 * @param filePath Ścieżka docelowego pliku `.json`.
 * @param initialState Stan początkowy wybrany w edytorze.
 * @param blankSymbol Symbol pusty taśmy.
 * @param numStates Liczba stanów widocznych w UI.
 * @param alphabet Alfabet maszyny.
 * @return `true`, jeśli zapis do pliku się powiódł.
 */
bool saveInstructionsToFile(const TuringMachine& machine, const QString& filePath, const QString& initialState, char blankSymbol, int numStates, const std::vector<char>& alphabet);

/**
 * @brief Wczytuje definicję maszyny z pliku JSON.
 * @param filePath Ścieżka pliku do odczytu.
 * @return MachineData z ustawionym `success` oraz ewentualnym `errorMessage`.
 */
MachineData loadInstructionsFromFile(const QString& filePath);

/**
 * @brief Odtwarza definicję maszyny bezpośrednio z dokumentu JSON.
 * @param jsonData Zawartość JSON w tym samym formacie, którego używają pliki zapisu.
 * @return MachineData z ustawionym `success` oraz ewentualnym `errorMessage`.
 */
MachineData loadInstructionsFromJson(const QByteArray& jsonData);

/**
 * @brief Zapisuje definicję maszyny w lokalnej bazie SQLite.
 *
 * Jeśli maszyna o tej nazwie istnieje, jej rekord zostaje zastąpiony nową
 * definicją razem z kompletem przejść.
 *
 * @param machine Zbudowana maszyna zawierająca przejścia.
 * @param machineName Nazwa rekordu w bazie.
 * @param initialState Stan początkowy wybrany w edytorze.
 * @param blankSymbol Symbol pusty taśmy.
 * @param numStates Liczba stanów widocznych w UI.
 * @param alphabet Alfabet maszyny.
 * @return `true`, jeśli transakcja zapisu zakończyła się powodzeniem.
 */
bool saveMachineToDatabase(const TuringMachine& machine, const QString& machineName, const QString& initialState, char blankSymbol, int numStates, const std::vector<char>& alphabet);
/**
 * @brief Wczytuje definicję maszyny z lokalnej bazy SQLite.
 * @param machineName Nazwa maszyny do odczytu.
 * @return MachineData z ustawionym `success` oraz ewentualnym `errorMessage`.
 */
MachineData loadMachineFromDatabase(const QString& machineName);
/**
 * @brief Zwraca listę nazw maszyn zapisanych w bazie SQLite.
 * @return Posortowana lista nazw maszyn.
 */
QStringList getMachineNamesFromDatabase();

#endif // STORAGE_H
