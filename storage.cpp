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
#include <vector>

// --- STRUKTURA POMOCNICZA DO SORTOWANIA JSON-a ---
namespace {
struct TransData {
    std::string state;
    char readSym;
    QJsonObject obj;
};
}

bool saveInstructionsToFile(const TuringMachine& machine, const QString& filePath, const QString& initialState, char blankSymbol, int numStates, const std::vector<char>& alphabet) {
    std::vector<TransData> tempTrans;

    // 1. Zbieramy wszystkie instrukcje
    for (const auto& [key, transition] : machine.getTransitions()) {
        const auto& [state, readSymbol] = key;

        QJsonObject transitionObj;
        transitionObj["currentState"] = QString::fromStdString(state);
        transitionObj["readSymbol"] = QString(QChar(readSymbol));
        transitionObj["nextState"] = QString::fromStdString(transition.nextState);
        transitionObj["writeSymbol"] = QString(QChar(transition.writeSymbol));
        transitionObj["move"] = QString::fromStdString(moveToString(transition.move));

        tempTrans.push_back({state, readSymbol, transitionObj});
    }

    // 2. Sortowanie: Stan -> Znak (z blankiem na początku)
    auto getSymbolRank = [blankSymbol](char c) -> int {
        if (c == blankSymbol) return 0;
        return c + 256;
    };

    std::sort(tempTrans.begin(), tempTrans.end(), [&](const TransData& a, const TransData& b) {
        if (a.state != b.state) {
            if (a.state.length() != b.state.length()) return a.state.length() < b.state.length();
            return a.state < b.state;
        }
        return getSymbolRank(a.readSym) < getSymbolRank(b.readSym);
    });

    QJsonArray transitionsArray;
    for (const auto& t : tempTrans) {
        transitionsArray.append(t.obj);
    }

    QJsonObject rootObject;
    rootObject["transitions"] = transitionsArray;
    rootObject["initialState"] = initialState;
    rootObject["blankSymbol"] = QString(QChar(blankSymbol));
    rootObject["numStates"] = numStates;

    QJsonArray alphabetArray;
    alphabetArray.append(QString(QChar(blankSymbol)));
    for (char c : alphabet) {
        if (c != blankSymbol) alphabetArray.append(QString(QChar(c)));
    }
    rootObject["alphabet"] = alphabetArray;

    QJsonDocument doc(rootObject);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(doc.toJson());
    file.close();
    return true;
}

MachineData loadInstructionsFromFile(const QString& filePath) {
    MachineData data;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        data.errorMessage = "Nie można otworzyć pliku.";
        return data;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject root = doc.object();

    data.initialState = root["initialState"].toString().toStdString();
    data.blankSymbol = root["blankSymbol"].toString()[0].toLatin1();
    data.numStates = root["numStates"].toInt();

    QJsonArray alphaArray = root["alphabet"].toArray();
    for (const auto& val : alphaArray) {
        data.alphabet.push_back(val.toString()[0].toLatin1());
    }

    QJsonArray transitionsArray = root["transitions"].toArray();
    for (const auto& val : transitionsArray) {
        QJsonObject t = val.toObject();
        data.transitions.push_back({
            t["currentState"].toString().toStdString(),
            t["readSymbol"].toString()[0].toLatin1(),
            t["nextState"].toString().toStdString(),
            t["writeSymbol"].toString()[0].toLatin1(),
            t["move"].toString().toStdString()
        });
    }

    data.success = true;
    return data;
}

// --- BAZA DANYCH ---
QSqlDatabase getDatabase() {
    if (QSqlDatabase::contains("turing_connection")) {
        return QSqlDatabase::database("turing_connection");
    } else {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "turing_connection");
        db.setDatabaseName("machines.db");
        return db;
    }
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
bool saveMachineToDatabase(const TuringMachine& machine, const QString& machineName, const QString& initialState, char blankSymbol, int numStates, const std::vector<char>& alphabet) {
    QSqlDatabase db = getDatabase();
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.exec("CREATE TABLE IF NOT EXISTS machines (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE, num_states INTEGER, initial_state TEXT, blank_symbol TEXT, alphabet TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS transitions (machine_id INTEGER, current_state TEXT, read_symbol TEXT, next_state TEXT, write_symbol TEXT, move_direction TEXT)");

    query.prepare("SELECT id FROM machines WHERE name = :name");
    query.bindValue(":name", machineName);
    if (query.exec() && query.next()) {
        int id = query.value(0).toInt();
        QSqlQuery del(db);
        del.prepare("DELETE FROM transitions WHERE machine_id = ?");
        del.addBindValue(id);
        del.exec();
        del.prepare("DELETE FROM machines WHERE id = ?");
        del.addBindValue(id);
        del.exec();
    }

    QString alphaStr;
    alphaStr += blankSymbol;
    for (char c : alphabet) if (c != blankSymbol) alphaStr += c;

    query.prepare("INSERT INTO machines (name, num_states, initial_state, blank_symbol, alphabet) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(machineName);
    query.addBindValue(numStates);
    query.addBindValue(initialState);
    query.addBindValue(QString(QChar(blankSymbol)));
    query.addBindValue(alphaStr);
    query.exec();

    int machineId = query.lastInsertId().toInt();
    for (const auto& [key, transition] : machine.getTransitions()) {
        QSqlQuery t(db);
        t.prepare("INSERT INTO transitions VALUES (?, ?, ?, ?, ?, ?)");
        t.addBindValue(machineId);
        t.addBindValue(QString::fromStdString(key.first));
        t.addBindValue(QString(QChar(key.second)));
        t.addBindValue(QString::fromStdString(transition.nextState));
        t.addBindValue(QString(QChar(transition.writeSymbol)));
        t.addBindValue(QString::fromStdString(moveToString(transition.move)));
        t.exec();
    }
    db.close();
    return true;
}

MachineData loadMachineFromDatabase(const QString& machineName) {
    MachineData data;
    QSqlDatabase db = getDatabase();
    if (!db.open()) return data;

    QSqlQuery q(db);
    q.prepare("SELECT id, num_states, initial_state, blank_symbol, alphabet FROM machines WHERE name = ?");
    q.addBindValue(machineName);
    if (q.exec() && q.next()) {
        int id = q.value(0).toInt();
        data.numStates = q.value(1).toInt();
        data.initialState = q.value(2).toString().toStdString();
        data.blankSymbol = q.value(3).toString()[0].toLatin1();

        QString alpha = q.value(4).toString();
        for (QChar c : alpha) data.alphabet.push_back(c.toLatin1());

        QSqlQuery t(db);
        t.prepare("SELECT current_state, read_symbol, next_state, write_symbol, move_direction FROM transitions WHERE machine_id = ?");
        t.addBindValue(id);
        if (t.exec()) {
            while (t.next()) {
                data.transitions.push_back({
                    t.value(0).toString().toStdString(),
                    t.value(1).toString()[0].toLatin1(),
                    t.value(2).toString().toStdString(),
                    t.value(3).toString()[0].toLatin1(),
                    t.value(4).toString().toStdString()
                });
            }
        }
    }
    db.close();
    data.success = true;
    return data;
}

