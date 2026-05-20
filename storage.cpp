#include "storage.h"
#include "TuringMachine.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
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

// =================================================================
// OBSŁUGA BAZY DANYCH SQLITE
// =================================================================

QSqlDatabase getDatabase() {
    if (QSqlDatabase::contains("turing_connection")) {
        return QSqlDatabase::database("turing_connection");
    } else {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "turing_connection");
        db.setDatabaseName("machines.db");
        return db;
    }
}

bool saveMachineToDatabase(const TuringMachine& machine, const QString& machineName, const QString& initialState, char blankSymbol, int numStates, const std::set<char>& alphabet) {
    QSqlDatabase db = getDatabase();
    if (!db.open()) return false;

    QSqlQuery query(db);
    // Tworzenie tabel, jeśli to pierwsze uruchomienie
    query.exec("CREATE TABLE IF NOT EXISTS machines (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE, num_states INTEGER, initial_state TEXT, blank_symbol TEXT, alphabet TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS transitions (machine_id INTEGER, current_state TEXT, read_symbol TEXT, next_state TEXT, write_symbol TEXT, move_direction TEXT)");

    // Usunięcie starej maszyny o tej samej nazwie (żeby zrobić nadpisywanie "Save")
    query.prepare("SELECT id FROM machines WHERE name = :name");
    query.bindValue(":name", machineName);
    int machineId = -1;
    if (query.exec() && query.next()) {
        machineId = query.value(0).toInt();
        QSqlQuery deleteTrans(db);
        deleteTrans.prepare("DELETE FROM transitions WHERE machine_id = :id");
        deleteTrans.bindValue(":id", machineId);
        deleteTrans.exec();

        QSqlQuery deleteMach(db);
        deleteMach.prepare("DELETE FROM machines WHERE id = :id");
        deleteMach.bindValue(":id", machineId);
        deleteMach.exec();
    }

    // Zapis do tabeli głównej (machines)
    QString alphaStr;
    for (char c : alphabet) alphaStr += c;

    query.prepare("INSERT INTO machines (name, num_states, initial_state, blank_symbol, alphabet) VALUES (:name, :states, :initial, :blank, :alpha)");
    query.bindValue(":name", machineName);
    query.bindValue(":states", numStates);
    query.bindValue(":initial", initialState);
    query.bindValue(":blank", QString(QChar(blankSymbol)));
    query.bindValue(":alpha", alphaStr);

    if (!query.exec()) {
        db.close();
        return false;
    }

    machineId = query.lastInsertId().toInt();

    // Szybki zapis instrukcji w jednej transakcji
    db.transaction();
    QSqlQuery transQuery(db);
    transQuery.prepare("INSERT INTO transitions (machine_id, current_state, read_symbol, next_state, write_symbol, move_direction) VALUES (:m_id, :curr, :read, :next, :write, :move)");

    for (const auto& [key, transition] : machine.getTransitions()) {
        transQuery.bindValue(":m_id", machineId);
        transQuery.bindValue(":curr", QString::fromStdString(key.first));
        transQuery.bindValue(":read", QString(QChar(key.second)));
        transQuery.bindValue(":next", QString::fromStdString(transition.nextState));
        transQuery.bindValue(":write", QString(QChar(transition.writeSymbol)));
        transQuery.bindValue(":move", QString::fromStdString(moveToString(transition.move)));
        transQuery.exec();
    }
    db.commit();
    db.close();
    return true;
}

QStringList getMachineNamesFromDatabase() {
    QStringList names;
    QSqlDatabase db = getDatabase();
    if (db.open()) {
        QSqlQuery query("SELECT name FROM machines", db);
        while (query.next()) {
            names.append(query.value(0).toString());
        }
        db.close();
    }
    return names;
}

MachineData loadMachineFromDatabase(const QString& machineName) {
    MachineData data;
    QSqlDatabase db = getDatabase();
    if (!db.open()) {
        data.errorMessage = "Nie udało się połączyć z bazą danych.";
        return data;
    }

    QSqlQuery query(db);
    query.prepare("SELECT id, num_states, initial_state, blank_symbol, alphabet FROM machines WHERE name = :name");
    query.bindValue(":name", machineName);

    if (!query.exec() || !query.next()) {
        data.errorMessage = "Nie znaleziono maszyny w bazie.";
        db.close();
        return data;
    }

    int machineId = query.value(0).toInt();
    data.numStates = query.value(1).toInt();
    data.initialState = query.value(2).toString().toStdString();

    QString blankStr = query.value(3).toString();
    if (!blankStr.isEmpty()) data.blankSymbol = blankStr[0].toLatin1();

    QString alphaStr = query.value(4).toString();
    for (QChar c : alphaStr) {
        data.alphabet.insert(c.toLatin1());
    }

    // Odczyt tabeli przejść
    QSqlQuery transQuery(db);
    transQuery.prepare("SELECT current_state, read_symbol, next_state, write_symbol, move_direction FROM transitions WHERE machine_id = :id");
    transQuery.bindValue(":id", machineId);
    if (transQuery.exec()) {
        while (transQuery.next()) {
            std::string curr = transQuery.value(0).toString().toStdString();
            char readSym = transQuery.value(1).toString()[0].toLatin1();
            std::string next = transQuery.value(2).toString().toStdString();
            char writeSym = transQuery.value(3).toString()[0].toLatin1();
            std::string move = transQuery.value(4).toString().toStdString();

            data.transitions.push_back({curr, readSym, next, writeSym, move});
        }
    }

    db.close();
    data.success = true;
    return data;
}