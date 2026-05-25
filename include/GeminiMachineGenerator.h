#pragma once

/**
 * @file GeminiMachineGenerator.h
 * @brief Klient Gemini API generujący definicje maszyn Turinga.
 */

#include "storage.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

/**
 * @brief Wysyła opis zadania do Gemini i zamienia odpowiedź na MachineData.
 */
class GeminiMachineGenerator : public QObject
{
public:
    struct Result
    {
        bool success = false;
        MachineData machineData;
        QString errorMessage;
        QString rawResponse;
    };

    struct TierConfig
    {
        QString id;
        QString displayName;
        QString primaryModel;
        QString fallbackModel;
    };

    explicit GeminiMachineGenerator(QObject *parent = nullptr);

    /**
     * @brief Uruchamia asynchroniczne generowanie maszyny.
     * @param taskDescription Opis maszyny podany przez użytkownika.
     * @param apiKey Klucz Gemini API.
     * @param modelName Nazwa modelu Gemini, np. `gemini-3.5-flash`.
     * @param thinkingEnabled Czy poprosić Gemini o wyższy poziom myślenia.
     * @param onFinished Funkcja zwrotna wywoływana po odpowiedzi, błędzie lub anulowaniu.
     * @return Aktywny obiekt odpowiedzi Qt, którego można użyć do anulowania żądania.
     */
    QNetworkReply *generate(
        const QString &taskDescription,
        const QString &apiKey,
        const QString &modelName,
        bool thinkingEnabled,
        std::function<void(const Result &)> onFinished);

    static QList<TierConfig> tierConfigs();
    static TierConfig tierConfig(const QString &tierId);
    static QStringList availableTierIds();
    static QString tierDisplayName(const QString &tierId);
    static bool shouldTryFallback(const Result &result);

private:
    QString buildPrompt(const QString &taskDescription) const;
    QString normalizeModelName(const QString &modelName) const;
    Result parseResponse(const QByteArray &responseBody, int httpStatus) const;

    QNetworkAccessManager *networkManager;
};
