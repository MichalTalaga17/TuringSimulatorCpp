#ifndef STORAGE_H
#define STORAGE_H
#include <QString>
#include <vector>
#include <set>
#include <tuple>
#include <string>

// Deklaracja zapowiadająca, żeby nie includować całego pliku nagłówkowego
class TuringMachine;

// Struktura na dane wyciągnięte z pliku (niezależna od UI)
struct MachineData {
    int numStates = 0;
    std::set<char> alphabet;
    std::vector<std::tuple<std::string, char, std::string, char, std::string>> transitions;

    // --- NOWE POLA ---
    std::string initialState;
    char blankSymbol = '_';

    bool success = false;
    QString errorMessage = "";
};

// Zmieniamy sygnaturę, żeby przyjmowała stan i symbol pusty z UI
bool saveInstructionsToFile(const TuringMachine& machine, const QString& filePath, const QString& initialState, char blankSymbol);

// Nowa funkcja do czytania
MachineData loadInstructionsFromFile(const QString& filePath);

bool saveInstructionsToFile(const TuringMachine& machine, const QString& filePath);

#endif // STORAGE_H
