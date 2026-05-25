#pragma once

/**
 * @file GeminiApiKeysDialog.h
 * @brief Modal ustawień kluczy API dla wariantów Gemini.
 */

#include <QDialog>
#include <QString>

class QLineEdit;
class QSettings;
class QWidget;

/**
 * @brief Widok konfiguracji kluczy API dla wariantów Free i Pro.
 */
class GeminiApiKeysDialog : public QDialog
{
public:
    explicit GeminiApiKeysDialog(
        const QString &initialFreeApiKey,
        const QString &initialProApiKey,
        QWidget *parent = nullptr);

    QString freeApiKey() const;
    QString proApiKey() const;

    static QString apiKeyForTier(const QString &tierId, const QSettings &settings);
    static bool editStoredKeys(QWidget *parent, QSettings &settings);

private:
    QLineEdit *freeApiKeyField;
    QLineEdit *proApiKeyField;
};
