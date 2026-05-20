#include "storage.h"
#include "TuringMachine.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <algorithm>

bool saveInstructionsToFile(const TuringMachine& machine, const QString& filePath, const QString& initialState, char blankSymbol, int numStates, const std::set<char>& alphabet) {
    QJsonArray transitionsArray;

    for (const auto& [key, transition] : machine.getTransitions()) {
        const auto& [state, readSymbol] = key;

        QJsonObject transitionObj;
        transitionObj["currentState"] = QString::fromStdString(state);
        transitionObj["readSymbol"] = QString(QChar(readSymbol));

        transitionObj["nextState"] = QString::fromStdString(transition.nextState);
        transitionObj["writeSymbol"] = QString(QChar(transition.writeSymbol));
        transitionObj["move"] = QString::fromStdString(moveToString(transition.move));

        transitionsArray.append(transitionObj);
    }

    QJsonObject rootObject;
    rootObject["transitions"] = transitionsArray;
    rootObject["initialState"] = initialState;
    rootObject["blankSymbol"] = QString(QChar(blankSymbol));

    // --- ZAPISUJEMY ILOŚĆ STANÓW I ALFABET ---
    rootObject["numStates"] = numStates;

    QJsonArray alphabetArray;
    for (char c : alphabet) {
        alphabetArray.append(QString(QChar(c)));
    }
    rootObject["alphabet"] = alphabetArray;

    QJsonDocument doc(rootObject);
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

MachineData loadInstructionsFromFile(const QString& filePath) {
    MachineData data;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        data.errorMessage = "Nie można otworzyć pliku do odczytu.";
        return data;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) {
        data.errorMessage = "Plik nie jest poprawnym formatem JSON.";
        return data;
    }

    QJsonObject root = doc.object();

    data.initialState = root["initialState"].toString().toStdString();
    QString blankStr = root["blankSymbol"].toString();
    if (!blankStr.isEmpty()) {
        data.blankSymbol = blankStr[0].toLatin1();
    }

    // --- ODCZYTUJEMY ILOŚĆ STANÓW I ALFABET (Z ZABEZPIECZENIEM DLA STARYCH PLIKÓW) ---
    bool hasExplicitNumStates = root.contains("numStates");
    if (hasExplicitNumStates) {
        data.numStates = root["numStates"].toInt();
    }

    bool hasExplicitAlphabet = root.contains("alphabet");
    if (hasExplicitAlphabet) {
        QJsonArray alphaArray = root["alphabet"].toArray();
        for (int i = 0; i < alphaArray.size(); ++i) {
            QString s = alphaArray[i].toString();
            if (!s.isEmpty()) {
                data.alphabet.insert(s[0].toLatin1());
            }
        }
    }

    QJsonArray transitionsArray = root["transitions"].toArray();
    int maxStateNumber = 0;

    // Skanujemy instrukcje przejść
    for (int i = 0; i < transitionsArray.size(); ++i) {
        QJsonObject tObj = transitionsArray[i].toObject();

        QString currentState = tObj["currentState"].toString();
        QString readSymbolStr = tObj["readSymbol"].toString();
        QString nextState = tObj["nextState"].toString();
        QString writeSymbolStr = tObj["writeSymbol"].toString();
        QString move = tObj["move"].toString();

        if (readSymbolStr.isEmpty() || writeSymbolStr.isEmpty()) continue;

        char readSymbol = readSymbolStr[0].toLatin1();
        char writeSymbol = writeSymbolStr[0].toLatin1();

        // Jeśli stary plik nie miał zapisanego alfabetu, budujemy go zgadując z przejść
        if (!hasExplicitAlphabet) {
            data.alphabet.insert(readSymbol);
            data.alphabet.insert(writeSymbol);
        }

        // Jeśli stary plik nie miał zapisanego numStates, szukamy najwyższego numeru
        if (!hasExplicitNumStates) {
            if (currentState.startsWith("q")) {
                maxStateNumber = std::max(maxStateNumber, currentState.mid(1).toInt());
            }
            if (nextState.startsWith("q")) {
                maxStateNumber = std::max(maxStateNumber, nextState.mid(1).toInt());
            }
        }

        // Zapisujemy przejście
        data.transitions.push_back({
            currentState.toStdString(),
            readSymbol,
            nextState.toStdString(),
            writeSymbol,
            move.toStdString()
        });
    }

    // Uzupełnienie zgadywania dla starych plików
    if (!hasExplicitNumStates) {
        data.numStates = maxStateNumber + 1;
    }

    data.success = true;
    return data;
}