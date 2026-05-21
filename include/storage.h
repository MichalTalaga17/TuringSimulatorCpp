#ifndef STORAGE_H
#define STORAGE_H
#include <QString>
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include <QStringList>

// Deklaracja zapowiadająca, żeby nie includować całego pliku nagłówkowego
class TuringMachine;

// Struktura na dane wyciągnięte z pliku (niezależna od UI)
struct MachineData {
    int numStates = 0;
    std::vector<char> alphabet;
    std::vector<std::tuple<std::string, char, std::string, char, std::string>> transitions;

    // --- NOWE POLA ---
    std::string initialState;
    char blankSymbol = '_';

    bool success = false;
    QString errorMessage = "";
};

// Sygnatura z dodaną liczbą stanów i alfabetem
bool saveInstructionsToFile(const TuringMachine& machine, const QString& filePath, const QString& initialState, char blankSymbol, int numStates, const std::vector<char>& alphabet);

// Funkcja do czytania
MachineData loadInstructionsFromFile(const QString& filePath);

// --- BAZA DANYCH (SQLite) ---
bool saveMachineToDatabase(const TuringMachine& machine, const QString& machineName, const QString& initialState, char blankSymbol, int numStates, const std::vector<char>& alphabet);
MachineData loadMachineFromDatabase(const QString& machineName);
QStringList getMachineNamesFromDatabase();

#endif // STORAGE_H