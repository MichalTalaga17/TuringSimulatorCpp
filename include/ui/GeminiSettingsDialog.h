#pragma once

/**
 * @file GeminiSettingsDialog.h
 * @brief Modal opisu zadania, wyboru wariantu Gemini i trybu myślenia.
 */

#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QTextEdit;
class QWidget;

/**
 * @brief Widok konfiguracji pojedynczego generowania przez Gemini API.
 */
class GeminiSettingsDialog : public QDialog
{
public:
    explicit GeminiSettingsDialog(
        const QString &initialDescription,
        const QString &initialTier,
        bool initialThinking,
        QWidget *parent = nullptr);

    QString taskDescription() const;
    QString tierId() const;
    bool thinkingEnabled() const;

private:
    void updateAcceptButton();

    QTextEdit *descriptionEdit;
    QComboBox *tierCombo;
    QCheckBox *thinkingCheckBox;
    QDialogButtonBox *buttonBox;
};
