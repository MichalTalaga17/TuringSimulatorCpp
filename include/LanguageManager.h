#pragma once

/**
 * @file LanguageManager.h
 * @brief Prosty menedżer języka interfejsu aplikacji.
 */

#include <QString>

class LanguageManager
{
public:
    enum class Language
    {
        Polish,
        English
    };

    static Language currentLanguage();
    static void setCurrentLanguage(Language language);
    static void loadFromSettings();
    static QString text(const char *key);
    static QString text(const QString &key);
    static QString languageCode(Language language);

    static QString moveLeftText();
    static QString moveRightText();
    static QString moveStayText();
    static QString moveStopText();
    static QString localizedMoveText(QString moveText);
};
