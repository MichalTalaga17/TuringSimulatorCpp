#include "storage.h"

#include "TuringMachine.h"

#include <QDir>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace
{
QSqlDatabase getDatabase()
{
    if (QSqlDatabase::contains("turing_connection"))
    {
        return QSqlDatabase::database("turing_connection");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "turing_connection");

    QString dataDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDirPath.isEmpty())
    {
        dataDirPath = QDir::currentPath();
    }

    QDir dataDir(dataDirPath);
    if (!dataDir.exists())
    {
        dataDir.mkpath(".");
    }

    db.setDatabaseName(dataDir.filePath("machines.db"));
    return db;
}

bool ensureMachineTables(QSqlDatabase &db)
{
    QSqlQuery machinesQuery(db);
    if (!machinesQuery.exec(
            "CREATE TABLE IF NOT EXISTS machines ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT UNIQUE, "
            "num_states INTEGER, "
            "initial_state TEXT, "
            "blank_symbol TEXT, "
            "alphabet TEXT)"))
    {
        return false;
    }

    QSqlQuery transitionsQuery(db);
    return transitionsQuery.exec(
        "CREATE TABLE IF NOT EXISTS transitions ("
        "machine_id INTEGER, "
        "current_state TEXT, "
        "read_symbol TEXT, "
        "next_state TEXT, "
        "write_symbol TEXT, "
        "move_direction TEXT)");
}

bool readSingleDatabaseChar(const QString &text, const QString &fieldName, char &value, MachineData &data)
{
    if (text.size() != 1)
    {
        data.errorMessage = QString("Niepoprawna wartość pola %1 w bazie danych.").arg(fieldName);
        return false;
    }

    value = text[0].toLatin1();
    return true;
}

MachineData failLoad(MachineData data, const QString &message)
{
    data.success = false;
    data.errorMessage = message;
    data.transitions.clear();
    data.alphabet.clear();
    return data;
}
} // koniec przestrzeni nazw

QStringList getMachineNamesFromDatabase()
{
    QStringList names;
    QSqlDatabase db = getDatabase();
    if (!db.open())
    {
        return names;
    }

    if (!ensureMachineTables(db))
    {
        db.close();
        return names;
    }

    QSqlQuery query(db);
    if (query.exec("SELECT name FROM machines ORDER BY lower(name)"))
    {
        while (query.next())
        {
            names.append(query.value(0).toString());
        }
    }

    db.close();
    return names;
}

bool saveMachineToDatabase(
    const TuringMachine &machine,
    const QString &machineName,
    const QString &initialState,
    char blankSymbol,
    int numStates,
    const std::vector<char> &alphabet)
{
    QSqlDatabase db = getDatabase();
    if (!db.open() || !ensureMachineTables(db) || !db.transaction())
    {
        db.close();
        return false;
    }

    auto rollbackAndClose = [&db]()
    {
        db.rollback();
        db.close();
        return false;
    };

    QSqlQuery query(db);
    query.prepare("SELECT id FROM machines WHERE name = ?");
    query.addBindValue(machineName);
    if (!query.exec())
    {
        return rollbackAndClose();
    }

    if (query.next())
    {
        const int id = query.value(0).toInt();

        QSqlQuery deleteTransitions(db);
        deleteTransitions.prepare("DELETE FROM transitions WHERE machine_id = ?");
        deleteTransitions.addBindValue(id);
        if (!deleteTransitions.exec())
        {
            return rollbackAndClose();
        }

        QSqlQuery deleteMachine(db);
        deleteMachine.prepare("DELETE FROM machines WHERE id = ?");
        deleteMachine.addBindValue(id);
        if (!deleteMachine.exec())
        {
            return rollbackAndClose();
        }
    }

    QString alphabetText;
    alphabetText += QChar(blankSymbol);
    for (char symbol : alphabet)
    {
        if (symbol != blankSymbol)
        {
            alphabetText += QChar(symbol);
        }
    }

    QSqlQuery insertMachine(db);
    insertMachine.prepare(
        "INSERT INTO machines (name, num_states, initial_state, blank_symbol, alphabet) "
        "VALUES (?, ?, ?, ?, ?)");
    insertMachine.addBindValue(machineName);
    insertMachine.addBindValue(numStates);
    insertMachine.addBindValue(initialState);
    insertMachine.addBindValue(QString(QChar(blankSymbol)));
    insertMachine.addBindValue(alphabetText);
    if (!insertMachine.exec())
    {
        return rollbackAndClose();
    }

    const int machineId = insertMachine.lastInsertId().toInt();
    for (const auto &[key, transition] : machine.getTransitions())
    {
        QSqlQuery insertTransition(db);
        insertTransition.prepare(
            "INSERT INTO transitions "
            "(machine_id, current_state, read_symbol, next_state, write_symbol, move_direction) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        insertTransition.addBindValue(machineId);
        insertTransition.addBindValue(QString::fromStdString(key.first));
        insertTransition.addBindValue(QString(QChar(key.second)));
        insertTransition.addBindValue(QString::fromStdString(transition.nextState));
        insertTransition.addBindValue(QString(QChar(transition.writeSymbol)));
        insertTransition.addBindValue(QString::fromStdString(moveToString(transition.move)));

        if (!insertTransition.exec())
        {
            return rollbackAndClose();
        }
    }

    if (!db.commit())
    {
        return rollbackAndClose();
    }

    db.close();
    return true;
}

MachineData loadMachineFromDatabase(const QString &machineName)
{
    MachineData data;
    QSqlDatabase db = getDatabase();
    if (!db.open())
    {
        return failLoad(data, "Nie można otworzyć bazy danych.");
    }

    auto failAndClose = [&db](MachineData currentData, const QString &message)
    {
        db.close();
        return failLoad(currentData, message);
    };

    if (!ensureMachineTables(db))
    {
        return failAndClose(data, "Nie można przygotować tabel bazy danych.");
    }

    QSqlQuery query(db);
    query.prepare("SELECT id, num_states, initial_state, blank_symbol, alphabet FROM machines WHERE name = ?");
    query.addBindValue(machineName);
    if (!query.exec())
    {
        return failAndClose(data, "Nie udało się odczytać maszyny z bazy danych.");
    }

    if (!query.next())
    {
        return failAndClose(data, "Nie znaleziono maszyny w bazie danych.");
    }

    const int machineId = query.value(0).toInt();
    data.numStates = query.value(1).toInt();
    data.initialState = query.value(2).toString().toStdString();

    if (!readSingleDatabaseChar(query.value(3).toString(), "blank_symbol", data.blankSymbol, data))
    {
        return failAndClose(data, data.errorMessage);
    }

    const QString alphabetText = query.value(4).toString();
    for (QChar symbol : alphabetText)
    {
        data.alphabet.push_back(symbol.toLatin1());
    }

    QSqlQuery transitionsQuery(db);
    transitionsQuery.prepare(
        "SELECT current_state, read_symbol, next_state, write_symbol, move_direction "
        "FROM transitions WHERE machine_id = ?");
    transitionsQuery.addBindValue(machineId);
    if (!transitionsQuery.exec())
    {
        return failAndClose(data, "Nie udało się odczytać instrukcji z bazy danych.");
    }

    while (transitionsQuery.next())
    {
        char readSymbol = data.blankSymbol;
        char writeSymbol = data.blankSymbol;
        if (!readSingleDatabaseChar(transitionsQuery.value(1).toString(), "read_symbol", readSymbol, data) ||
            !readSingleDatabaseChar(transitionsQuery.value(3).toString(), "write_symbol", writeSymbol, data))
        {
            return failAndClose(data, data.errorMessage);
        }

        data.transitions.push_back({
            transitionsQuery.value(0).toString().toStdString(),
            readSymbol,
            transitionsQuery.value(2).toString().toStdString(),
            writeSymbol,
            transitionsQuery.value(4).toString().toStdString()});
    }

    db.close();
    data.success = true;
    return data;
}
