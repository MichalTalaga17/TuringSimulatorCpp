#pragma once

#include <QString>
#include <QHash>
#include <QObject>

class LanguageManager : public QObject
{
    Q_OBJECT
public:
    enum class Language {
        Polish,
        English
    };

    static LanguageManager& instance();

    QString getString(const QString& key) const;
    void setLanguage(Language lang);
    Language currentLanguage() const;

signals:
    void languageChanged();

private:
    LanguageManager();
    ~LanguageManager() = default;

    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;

    void loadDictionaries();

    Language m_currentLanguage;
    QHash<QString, QString> m_plDict;
    QHash<QString, QString> m_enDict;
};

// Makro pomocnicze dla krótszego zapisu
#define TR(key) LanguageManager::instance().getString(key)
