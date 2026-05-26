#include "GeminiMachineGenerator.h"

#include "LanguageManager.h"

#include <QList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace
{
constexpr int requestTimeoutMs = 90000;
constexpr int maxGeneratedStates = 30;

bool isAllowedSymbol(char symbol)
{
    const unsigned char value = static_cast<unsigned char>(symbol);
    return value >= 33 && value <= 126 && symbol != ',';
}

void addUniqueSymbol(std::vector<char> &alphabet, char symbol)
{
    if (std::find(alphabet.begin(), alphabet.end(), symbol) == alphabet.end())
    {
        alphabet.push_back(symbol);
    }
}

QString stateName(int index)
{
    return QString("q%1").arg(index);
}

int stateIndex(const std::string &state)
{
    if (state.size() < 2 || state[0] != 'q')
    {
        return -1;
    }

    int value = 0;
    for (std::size_t index = 1; index < state.size(); ++index)
    {
        const unsigned char character = static_cast<unsigned char>(state[index]);
        if (!std::isdigit(character))
        {
            return -1;
        }

        value = (value * 10) + (state[index] - '0');
        if (value >= maxGeneratedStates)
        {
            return value;
        }
    }

    return value;
}

bool normalizeMove(std::string &move)
{
    const QString text = QString::fromStdString(move).trimmed().toUpper();
    if (text == "L" || text == "LEFT")
    {
        move = "L";
        return true;
    }

    if (text == "R" || text == "P" || text == "RIGHT")
    {
        move = "R";
        return true;
    }

    if (text == "_" || text == "S" || text == "STAY" || text == "NONE")
    {
        move = "Stay";
        return true;
    }

    if (text == "STOP" || text == "HALT" || text == "HALTED")
    {
        move = "Stop";
        return true;
    }

    return false;
}

QString candidateTextFromResponse(const QJsonObject &root)
{
    const QJsonArray candidates = root.value("candidates").toArray();
    if (candidates.isEmpty() || !candidates.first().isObject())
    {
        return {};
    }

    const QJsonObject firstCandidate = candidates.first().toObject();
    const QJsonObject content = firstCandidate.value("content").toObject();
    const QJsonArray parts = content.value("parts").toArray();

    QString text;
    for (const QJsonValue &partValue : parts)
    {
        if (partValue.isObject())
        {
            text += partValue.toObject().value("text").toString();
        }
    }

    return text;
}

QString shortLogText(QString text, int maxLength = 160)
{
    text = text.simplified();
    if (text.size() <= maxLength)
    {
        return text;
    }

    return text.left(maxLength - 3) + "...";
}

QString stripJsonMarkdownFence(QString text)
{
    text = text.trimmed();
    if (!text.startsWith("```"))
    {
        return text;
    }

    const int firstLineBreak = text.indexOf('\n');
    const int lastFence = text.lastIndexOf("```");
    if (firstLineBreak < 0 || lastFence <= firstLineBreak)
    {
        return text;
    }

    return text.mid(firstLineBreak + 1, lastFence - firstLineBreak - 1).trimmed();
}

MachineData parseMachineDataFromModelText(const QString &modelText, QString &errorMessage)
{
    const QString cleanedText = stripJsonMarkdownFence(modelText);
    MachineData data = loadInstructionsFromJson(cleanedText.toUtf8());
    if (data.success)
    {
        return data;
    }

    const int jsonStart = cleanedText.indexOf('{');
    const int jsonEnd = cleanedText.lastIndexOf('}');
    if (jsonStart >= 0 && jsonEnd > jsonStart)
    {
        const QString jsonObjectText = cleanedText.mid(jsonStart, jsonEnd - jsonStart + 1);
        data = loadInstructionsFromJson(jsonObjectText.toUtf8());
        if (data.success)
        {
            return data;
        }
    }

    errorMessage = data.errorMessage.isEmpty()
                       ? LanguageManager::text("gemini_invalid_json")
                       : data.errorMessage;
    return data;
}

bool normalizeGeneratedData(MachineData &data, QString &errorMessage)
{
    if (!isAllowedSymbol(data.blankSymbol))
    {
        errorMessage = LanguageManager::text("gemini_invalid_blank");
        return false;
    }

    if (data.numStates < 1)
    {
        data.numStates = 1;
    }

    if (data.numStates > maxGeneratedStates)
    {
        errorMessage = LanguageManager::text("gemini_too_many_states").arg(maxGeneratedStates);
        return false;
    }

    std::vector<char> normalizedAlphabet;
    normalizedAlphabet.push_back(data.blankSymbol);
    for (char symbol : data.alphabet)
    {
        if (!isAllowedSymbol(symbol))
        {
            errorMessage = LanguageManager::text("gemini_invalid_alphabet");
            return false;
        }

        if (symbol != data.blankSymbol)
        {
            addUniqueSymbol(normalizedAlphabet, symbol);
        }
    }

    int maxStateIndex = data.numStates - 1;
    const int initialIndex = stateIndex(data.initialState);
    if (initialIndex >= 0)
    {
        maxStateIndex = std::max(maxStateIndex, initialIndex);
    }

    using TransitionValue = std::tuple<std::string, char, std::string>;
    std::map<std::pair<std::string, char>, TransitionValue> normalizedTransitions;

    for (auto transition : data.transitions)
    {
        std::string currentState = std::get<0>(transition);
        char readSymbol = std::get<1>(transition);
        std::string nextState = std::get<2>(transition);
        char writeSymbol = std::get<3>(transition);
        std::string move = std::get<4>(transition);

        if (!normalizeMove(move))
        {
            errorMessage = LanguageManager::text("gemini_unsupported_move")
                               .arg(QString::fromStdString(std::get<4>(transition)));
            return false;
        }

        const int currentIndex = stateIndex(currentState);
        if (currentIndex < 0)
        {
            errorMessage = LanguageManager::text("gemini_invalid_state")
                               .arg(QString::fromStdString(currentState));
            return false;
        }
        maxStateIndex = std::max(maxStateIndex, currentIndex);

        int nextIndex = stateIndex(nextState);
        if (nextIndex < 0)
        {
            if (move == "Stop")
            {
                nextState = currentState;
            }
            else
            {
                errorMessage = LanguageManager::text("gemini_invalid_target_state")
                                   .arg(QString::fromStdString(nextState));
                return false;
            }
        }
        else
        {
            maxStateIndex = std::max(maxStateIndex, nextIndex);
        }

        if (!isAllowedSymbol(readSymbol) || !isAllowedSymbol(writeSymbol))
        {
            errorMessage = LanguageManager::text("gemini_invalid_transition_symbol");
            return false;
        }

        addUniqueSymbol(normalizedAlphabet, readSymbol);
        addUniqueSymbol(normalizedAlphabet, writeSymbol);
        normalizedTransitions[{currentState, readSymbol}] = {nextState, writeSymbol, move};
    }

    if (maxStateIndex >= maxGeneratedStates)
    {
        errorMessage = LanguageManager::text("gemini_state_out_of_range")
                           .arg(maxGeneratedStates - 1);
        return false;
    }

    data.numStates = std::max(data.numStates, maxStateIndex + 1);
    if (stateIndex(data.initialState) < 0 || stateIndex(data.initialState) >= data.numStates)
    {
        data.initialState = "q0";
    }

    data.alphabet = normalizedAlphabet;
    data.transitions.clear();

    for (int state = 0; state < data.numStates; ++state)
    {
        const std::string currentState = stateName(state).toStdString();
        for (char readSymbol : data.alphabet)
        {
            const auto transitionIt = normalizedTransitions.find({currentState, readSymbol});
            if (transitionIt == normalizedTransitions.end())
            {
                data.transitions.push_back({currentState, readSymbol, currentState, readSymbol, "Stop"});
                continue;
            }

            const auto &[nextState, writeSymbol, move] = transitionIt->second;
            data.transitions.push_back({currentState, readSymbol, nextState, writeSymbol, move});
        }
    }

    return true;
}
} // koniec przestrzeni nazw

GeminiMachineGenerator::GeminiMachineGenerator(QObject *parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this))
{
}

QList<GeminiMachineGenerator::TierConfig> GeminiMachineGenerator::tierConfigs()
{
    return {
        {"pro", LanguageManager::text("tier_pro"), "gemini-3.5-flash", {}},
        {"free", LanguageManager::text("tier_free"), "gemini-3.5-flash", "gemini-3.1-flash-lite"}};
}

GeminiMachineGenerator::TierConfig GeminiMachineGenerator::tierConfig(const QString &tierId)
{
    const QList<TierConfig> configs = tierConfigs();
    for (const TierConfig &config : configs)
    {
        if (config.id == tierId)
        {
            return config;
        }
    }

    return configs.first();
}

QStringList GeminiMachineGenerator::availableTierIds()
{
    QStringList ids;
    for (const TierConfig &config : tierConfigs())
    {
        ids.append(config.id);
    }
    return ids;
}

QString GeminiMachineGenerator::tierDisplayName(const QString &tierId)
{
    return tierConfig(tierId).displayName;
}

bool GeminiMachineGenerator::shouldTryFallback(const Result &result)
{
    if (result.success)
    {
        return false;
    }

    const QString error = result.errorMessage.toLower();
    return error.contains("quota") ||
           error.contains("limit") ||
           error.contains("rate") ||
           error.contains("not found") ||
           error.contains("not supported") ||
           error.contains("nieosi") ||
           error.contains("niedost") ||
           error.contains("nie znaleziono") ||
           error.contains("429") ||
           error.contains("404");
}

QNetworkReply *GeminiMachineGenerator::generate(
    const QString &taskDescription,
    const QString &apiKey,
    const QString &modelName,
    bool thinkingEnabled,
    std::function<void(const Result &)> onFinished)
{
    const QString model = normalizeModelName(modelName);
    const QString encodedModel = QString::fromLatin1(QUrl::toPercentEncoding(model));
    const QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent").arg(encodedModel));

    qInfo().noquote() << "[GeminiAI] Start żądania"
                      << "model=" + model
                      << "myślenie=" + QString(thinkingEnabled ? "włączone" : "wyłączone")
                      << "opis=\"" + shortLogText(taskDescription) + "\"";

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-goog-api-key", apiKey.toUtf8());

    QJsonObject textPart;
    textPart["text"] = buildPrompt(taskDescription);

    QJsonObject content;
    content["role"] = "user";
    content["parts"] = QJsonArray{textPart};

    QJsonObject generationConfig;
    generationConfig["temperature"] = 0.15;
    generationConfig["topP"] = 0.8;
    generationConfig["maxOutputTokens"] = 32768;
    generationConfig["responseMimeType"] = "application/json";

    QJsonObject thinkingConfig;
    thinkingConfig["thinkingLevel"] = thinkingEnabled ? "high" : "minimal";
    generationConfig["thinkingConfig"] = thinkingConfig;
    qInfo().noquote() << "[GeminiAI] Konfiguracja myślenia"
                      << "poziomMyślenia=" + thinkingConfig.value("thinkingLevel").toString();

    QJsonObject body;
    body["contents"] = QJsonArray{content};
    body["generationConfig"] = generationConfig;

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    qInfo().noquote() << "[GeminiAI] Żądanie wysłane do" << url.toString(QUrl::RemoveQuery);
    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);

    auto timedOut = std::make_shared<bool>(false);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply, timedOut]()
            {
        *timedOut = true;
        reply->abort(); });

    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, timedOut, onFinished]()
            {
        timeoutTimer->stop();

        Result result;
        if (*timedOut)
        {
            result.errorMessage = LanguageManager::text("gemini_timeout");
            qWarning().noquote() << "[GeminiAI] Przekroczono czas żądania";
        }
        else if (reply->error() != QNetworkReply::NoError)
        {
            const QByteArray responseBody = reply->readAll();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qWarning().noquote() << "[GeminiAI] Błąd sieci/API"
                                 << "httpStatus=" + QString::number(httpStatus)
                                 << "qtError=\"" + reply->errorString() + "\"";
            if (responseBody.isEmpty())
            {
                result.errorMessage = reply->errorString();
            }
            else
            {
                result = parseResponse(responseBody, httpStatus);
                if (result.errorMessage.isEmpty())
                {
                    result.errorMessage = reply->errorString();
                }
            }
        }
        else
        {
            const QByteArray responseBody = reply->readAll();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qInfo().noquote() << "[GeminiAI] Odebrano odpowiedź"
                              << "httpStatus=" + QString::number(httpStatus)
                              << "bytes=" + QString::number(responseBody.size());
            result = parseResponse(responseBody, httpStatus);
        }

        if (result.success)
        {
            qInfo().noquote() << "[GeminiAI] Maszyna sparsowana"
                              << "stany=" + QString::number(result.machineData.numStates)
                              << "rozmiarAlfabetu=" + QString::number(static_cast<int>(result.machineData.alphabet.size()))
                              << "przejścia=" + QString::number(static_cast<int>(result.machineData.transitions.size()));
        }
        if (!result.rawResponse.trimmed().isEmpty())
        {
            qInfo().noquote() << "[GeminiAI] Odpowiedź modelu:\n" + result.rawResponse.trimmed();
        }
        if (!result.success)
        {
            qWarning().noquote() << "[GeminiAI] Generowanie nie powiodło się"
                                 << "błąd=\"" + result.errorMessage + "\"";
        }

        if (onFinished)
        {
            onFinished(result);
        }

        reply->deleteLater(); });

    timeoutTimer->start(requestTimeoutMs);
    return reply;
}

QString GeminiMachineGenerator::buildPrompt(const QString &taskDescription) const
{
    return QString(R"(You generate deterministic one-tape Turing machines for a Qt simulator.
Return only valid JSON. Do not use Markdown. Do not add comments.
Return compact JSON, without pretty printing.

JSON schema:
{
  "numStates": 1,
  "alphabet": ["_", "0", "1"],
  "blankSymbol": "_",
  "initialState": "q0",
  "transitions": [
    {
      "currentState": "q0",
      "readSymbol": "0",
      "nextState": "q0",
      "writeSymbol": "1",
      "move": "R"
    }
  ]
}

Rules:
- Use state names exactly as q0, q1, q2, ... up to numStates - 1.
- numStates must be between 1 and 20.
- initialState must be one of the q-states.
- blankSymbol must be one printable ASCII character, usually "_".
- alphabet must include blankSymbol and every symbol used by any transition.
- Every symbol must be one printable ASCII character. Do not use comma or space as a symbol.
- Moves must be one of: "L", "R", "Stay", "Stop".
- Use "Stop" transitions to halt; this simulator does not have accepting/rejecting state metadata.
- Include only meaningful transitions. Missing state/symbol pairs will be treated as Stop transitions by the app.
- For a stopping transition, keep nextState equal to currentState unless another q-state is useful before stopping.
- Prefer a small, understandable machine.

User request:
%1)").arg(taskDescription.trimmed());
}

QString GeminiMachineGenerator::normalizeModelName(const QString &modelName) const
{
    QString normalized = modelName.trimmed();
    if (normalized.isEmpty())
    {
        normalized = tierConfig("pro").primaryModel;
    }

    if (normalized.startsWith("models/"))
    {
        normalized.remove(0, QString("models/").size());
    }

    return normalized;
}

GeminiMachineGenerator::Result GeminiMachineGenerator::parseResponse(const QByteArray &responseBody, int httpStatus) const
{
    Result result;

    QJsonParseError parseError;
    const QJsonDocument responseDocument = QJsonDocument::fromJson(responseBody, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDocument.isObject())
    {
        result.errorMessage = LanguageManager::text("gemini_invalid_api_response")
                                  .arg(parseError.errorString());
        return result;
    }

    const QJsonObject root = responseDocument.object();
    if (root.contains("error"))
    {
        const QJsonObject errorObject = root.value("error").toObject();
        result.errorMessage = errorObject.value("message").toString(LanguageManager::text("gemini_api_error"));
        return result;
    }

    if (httpStatus < 200 || httpStatus >= 300)
    {
        result.errorMessage = LanguageManager::text("gemini_http_status").arg(httpStatus);
        return result;
    }

    QString finishReason;
    const QJsonArray candidates = root.value("candidates").toArray();
    if (!candidates.isEmpty() && candidates.first().isObject())
    {
        finishReason = candidates.first().toObject().value("finishReason").toString();
        if (!finishReason.isEmpty())
        {
            qInfo().noquote() << "[GeminiAI] Powód zakończenia:" << finishReason;
        }
    }

    const QString modelText = candidateTextFromResponse(root);
    if (modelText.trimmed().isEmpty())
    {
        result.errorMessage = LanguageManager::text("gemini_empty_content");
        return result;
    }

    result.rawResponse = modelText;

    QString parseErrorMessage;
    MachineData generatedData = parseMachineDataFromModelText(modelText, parseErrorMessage);
    if (!generatedData.success)
    {
        if (finishReason == "MAX_TOKENS")
        {
            result.errorMessage = LanguageManager::text("gemini_max_tokens");
        }
        else
        {
            result.errorMessage = parseErrorMessage;
        }
        return result;
    }

    QString normalizationError;
    if (!normalizeGeneratedData(generatedData, normalizationError))
    {
        result.errorMessage = normalizationError;
        return result;
    }

    result.machineData = generatedData;
    result.success = true;
    return result;
}
