#include "storage.h"

#include "TuringMachine.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>
#include <vector>

namespace
{
struct SavedTransition
{
    std::string state;
    char readSymbol;
    QJsonObject object;
};

int symbolRank(char symbol, char blankSymbol)
{
    if (symbol == blankSymbol)
    {
        return 0;
    }

    return static_cast<unsigned char>(symbol) + 1;
}

bool readRequiredString(const QJsonObject &object, const char *field, QString &value, QString &errorMessage)
{
    const QJsonValue jsonValue = object.value(field);
    if (!jsonValue.isString() || jsonValue.toString().isEmpty())
    {
        errorMessage = QString("Brak lub niepoprawne pole: %1.").arg(field);
        return false;
    }

    value = jsonValue.toString();
    return true;
}

bool readRequiredChar(const QJsonObject &object, const char *field, char &value, QString &errorMessage)
{
    QString text;
    if (!readRequiredString(object, field, text, errorMessage))
    {
        return false;
    }

    if (text.size() != 1)
    {
        errorMessage = QString("Pole %1 musi zawierać dokładnie jeden znak.").arg(field);
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
} // namespace

bool saveInstructionsToFile(
    const TuringMachine &machine,
    const QString &filePath,
    const QString &initialState,
    char blankSymbol,
    int numStates,
    const std::vector<char> &alphabet)
{
    std::vector<SavedTransition> sortedTransitions;
    sortedTransitions.reserve(machine.getTransitions().size());

    for (const auto &[key, transition] : machine.getTransitions())
    {
        const auto &[state, readSymbol] = key;

        QJsonObject transitionObject;
        transitionObject["currentState"] = QString::fromStdString(state);
        transitionObject["readSymbol"] = QString(QChar(readSymbol));
        transitionObject["nextState"] = QString::fromStdString(transition.nextState);
        transitionObject["writeSymbol"] = QString(QChar(transition.writeSymbol));
        transitionObject["move"] = QString::fromStdString(moveToString(transition.move));

        sortedTransitions.push_back({state, readSymbol, transitionObject});
    }

    std::sort(
        sortedTransitions.begin(),
        sortedTransitions.end(),
        [blankSymbol](const SavedTransition &left, const SavedTransition &right)
        {
            if (left.state != right.state)
            {
                if (left.state.length() != right.state.length())
                {
                    return left.state.length() < right.state.length();
                }
                return left.state < right.state;
            }

            return symbolRank(left.readSymbol, blankSymbol) < symbolRank(right.readSymbol, blankSymbol);
        });

    QJsonArray transitionsArray;
    for (const SavedTransition &transition : sortedTransitions)
    {
        transitionsArray.append(transition.object);
    }

    QJsonArray alphabetArray;
    alphabetArray.append(QString(QChar(blankSymbol)));
    for (char symbol : alphabet)
    {
        if (symbol != blankSymbol)
        {
            alphabetArray.append(QString(QChar(symbol)));
        }
    }

    QJsonObject rootObject;
    rootObject["transitions"] = transitionsArray;
    rootObject["initialState"] = initialState;
    rootObject["blankSymbol"] = QString(QChar(blankSymbol));
    rootObject["numStates"] = numStates;
    rootObject["alphabet"] = alphabetArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }

    const QJsonDocument document(rootObject);
    return file.write(document.toJson()) != -1;
}

MachineData loadInstructionsFromFile(const QString &filePath)
{
    MachineData data;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return failLoad(data, "Nie można otworzyć pliku.");
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        return failLoad(data, "Niepoprawny JSON: " + parseError.errorString());
    }

    const QJsonObject root = document.object();
    QString initialState;
    QString errorMessage;
    if (!readRequiredString(root, "initialState", initialState, errorMessage))
    {
        return failLoad(data, errorMessage);
    }

    char blankSymbol = '_';
    if (!readRequiredChar(root, "blankSymbol", blankSymbol, errorMessage))
    {
        return failLoad(data, errorMessage);
    }

    const QJsonValue numStatesValue = root.value("numStates");
    if (!numStatesValue.isDouble() || numStatesValue.toInt() < 1)
    {
        return failLoad(data, "Pole numStates musi być dodatnią liczbą całkowitą.");
    }

    const QJsonValue alphabetValue = root.value("alphabet");
    if (!alphabetValue.isArray() || alphabetValue.toArray().isEmpty())
    {
        return failLoad(data, "Pole alphabet musi zawierać listę symboli.");
    }

    data.initialState = initialState.toStdString();
    data.blankSymbol = blankSymbol;
    data.numStates = numStatesValue.toInt();

    const QJsonArray alphabetArray = alphabetValue.toArray();
    for (int index = 0; index < alphabetArray.size(); ++index)
    {
        if (!alphabetArray[index].isString() || alphabetArray[index].toString().size() != 1)
        {
            return failLoad(data, QString("Niepoprawny symbol alfabetu pod indeksem %1.").arg(index));
        }

        const char symbol = alphabetArray[index].toString()[0].toLatin1();
        if (std::find(data.alphabet.begin(), data.alphabet.end(), symbol) == data.alphabet.end())
        {
            data.alphabet.push_back(symbol);
        }
    }

    if (std::find(data.alphabet.begin(), data.alphabet.end(), blankSymbol) == data.alphabet.end())
    {
        data.alphabet.insert(data.alphabet.begin(), blankSymbol);
    }

    const QJsonValue transitionsValue = root.value("transitions");
    if (!transitionsValue.isArray())
    {
        return failLoad(data, "Pole transitions musi zawierać listę instrukcji.");
    }

    const QJsonArray transitionsArray = transitionsValue.toArray();
    data.transitions.reserve(static_cast<std::size_t>(transitionsArray.size()));
    for (int index = 0; index < transitionsArray.size(); ++index)
    {
        if (!transitionsArray[index].isObject())
        {
            return failLoad(data, QString("Instrukcja #%1 nie jest obiektem JSON.").arg(index + 1));
        }

        const QJsonObject transitionObject = transitionsArray[index].toObject();
        QString currentState;
        QString nextState;
        QString move;
        char readSymbol = blankSymbol;
        char writeSymbol = blankSymbol;

        if (!readRequiredString(transitionObject, "currentState", currentState, errorMessage) ||
            !readRequiredChar(transitionObject, "readSymbol", readSymbol, errorMessage) ||
            !readRequiredString(transitionObject, "nextState", nextState, errorMessage) ||
            !readRequiredChar(transitionObject, "writeSymbol", writeSymbol, errorMessage) ||
            !readRequiredString(transitionObject, "move", move, errorMessage))
        {
            return failLoad(data, QString("Niepoprawna instrukcja #%1: %2").arg(index + 1).arg(errorMessage));
        }

        data.transitions.push_back({
            currentState.toStdString(),
            readSymbol,
            nextState.toStdString(),
            writeSymbol,
            move.toStdString()});
    }

    data.success = true;
    return data;
}
