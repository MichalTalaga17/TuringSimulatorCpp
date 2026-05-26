#include "MachineEditorTab.h"

#include "GeminiMachineGenerator.h"
#include "LanguageManager.h"
#include "ui/GeminiApiKeysDialog.h"
#include "ui/GeminiGenerationRunner.h"
#include "ui/GeminiSettingsDialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QSettings>

namespace
{
QString shortLogText(QString text, int maxLength = 160)
{
    text = text.simplified();
    if (text.size() <= maxLength)
    {
        return text;
    }

    return text.left(maxLength - 3) + "...";
}
} // koniec przestrzeni nazw

bool MachineEditorTab::resolveGeminiGenerationRequest(
    QWidget *parent,
    QString *description,
    QString *apiKey,
    QString *modelName,
    QString *fallbackModelName,
    bool *thinkingEnabled)
{
    if (!description || !apiKey || !modelName || !fallbackModelName || !thinkingEnabled)
    {
        return false;
    }

    QSettings settings;
    QString initialTier = settings.value("gemini/selectedTier", "pro").toString().trimmed();
    if (!GeminiMachineGenerator::availableTierIds().contains(initialTier))
    {
        initialTier = "pro";
    }

    const bool previousThinking = settings.value("gemini/thinkingEnabled", false).toBool();
    GeminiSettingsDialog settingsDialog(*description, initialTier, previousThinking, parent);
    if (settingsDialog.exec() != QDialog::Accepted)
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: odrzucono okno generowania";
        return false;
    }

    *description = settingsDialog.taskDescription();
    const QString tierId = settingsDialog.tierId();
    const GeminiMachineGenerator::TierConfig tierConfig = GeminiMachineGenerator::tierConfig(tierId);
    *modelName = tierConfig.primaryModel;
    *fallbackModelName = tierConfig.fallbackModel;
    *thinkingEnabled = settingsDialog.thinkingEnabled();
    if (description->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: pusty opis";
        return false;
    }
    if (tierId.isEmpty() || modelName->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: nie wybrano wariantu";
        return false;
    }

    *apiKey = GeminiApiKeysDialog::apiKeyForTier(tierId, settings);
    if (apiKey->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Źródło klucza API:"
                          << "wymagane okno ustawień";
        if (!GeminiApiKeysDialog::editStoredKeys(parent, settings))
        {
            qInfo().noquote() << "[GeminiAI] Anulowano: klucze API nie są skonfigurowane";
            return false;
        }

        *apiKey = GeminiApiKeysDialog::apiKeyForTier(tierId, settings);
    }
    else
    {
        qInfo().noquote() << "[GeminiAI] Źródło klucza API:"
                          << (tierId == "free" ? "wariant Free" : "wariant Pro");
    }

    if (apiKey->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: brakuje klucza API dla wybranego wariantu";
        QMessageBox::warning(
            parent,
            LanguageManager::text("gemini_api_title"),
            LanguageManager::text("gemini_missing_key")
                .arg(GeminiMachineGenerator::tierDisplayName(tierId)));
        return false;
    }

    settings.setValue("gemini/selectedTier", tierId);
    settings.setValue("gemini/thinkingEnabled", *thinkingEnabled);
    qInfo().noquote() << "[GeminiAI] Opis zaakceptowany"
                      << "opis=\"" + shortLogText(*description) + "\"";
    qInfo().noquote() << "[GeminiAI] Wybrany wariant:" << tierConfig.displayName;
    qInfo().noquote() << "[GeminiAI] Wybrany model:" << *modelName;
    if (!fallbackModelName->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Model zapasowy:" << *fallbackModelName;
    }
    qInfo().noquote() << "[GeminiAI] Wybrane myślenie:"
                      << (*thinkingEnabled ? "włączone" : "wyłączone");
    return true;
}

void MachineEditorTab::generateMachineWithAi()
{
    qInfo().noquote() << "[GeminiAI] Uruchomiono akcję interfejsu";

    if (aiGenerationInProgress)
    {
        qInfo().noquote() << "[GeminiAI] Zignorowano: generowanie już trwa";
        QMessageBox::information(this, LanguageManager::text("gemini_title"), LanguageManager::text("gemini_busy"));
        return;
    }

    QString description;
    QString apiKey;
    QString modelName;
    QString fallbackModelName;
    bool thinkingEnabled = false;
    if (!resolveGeminiGenerationRequest(this, &description, &apiKey, &modelName, &fallbackModelName, &thinkingEnabled))
    {
        return;
    }

    generateMachineWithAi(description, apiKey, modelName, fallbackModelName, thinkingEnabled);
}

void MachineEditorTab::generateMachineWithAi(
    const QString &description,
    const QString &apiKey,
    const QString &modelName,
    const QString &fallbackModelName,
    bool thinkingEnabled)
{
    if (aiGenerationInProgress)
    {
        qInfo().noquote() << "[GeminiAI] Zignorowano: generowanie już trwa";
        QMessageBox::information(this, LanguageManager::text("gemini_title"), LanguageManager::text("gemini_busy"));
        return;
    }

    const QString trimmedDescription = description.trimmed();
    const QString trimmedApiKey = apiKey.trimmed();
    const QString selectedModel = modelName.trimmed();
    const QString selectedFallbackModel = fallbackModelName.trimmed();

    if (trimmedDescription.isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: pusty opis";
        return;
    }

    if (trimmedApiKey.isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: pusty klucz API";
        return;
    }

    if (selectedModel.isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Anulowano: pusty model";
        return;
    }

    qInfo().noquote() << "[GeminiAI] Start generowania w edytorze"
                      << "model=" + selectedModel
                      << "myślenie=" + QString(thinkingEnabled ? "włączone" : "wyłączone")
                      << "opis=\"" + shortLogText(trimmedDescription) + "\"";

    aiGenerationInProgress = true;
    if (aiGenerateButton)
    {
        aiGenerateButton->setEnabled(false);
    }
    if (statusLabel)
    {
        setStatusText("status_ai_generating");
    }

    auto finishGeneration = [this](const QString &statusKey)
    {
        aiGenerationInProgress = false;
        if (aiGenerateButton)
        {
            aiGenerateButton->setEnabled(true);
        }
        if (!statusKey.isEmpty())
        {
            setStatusText(statusKey.toLatin1().constData());
        }
    };

    GeminiGenerationRequest request{
        trimmedDescription,
        trimmedApiKey,
        selectedModel,
        selectedFallbackModel,
        thinkingEnabled};

    runGeminiGenerationWithProgress(
        this,
        request,
        [this, finishGeneration](const GeminiMachineGenerator::Result &result)
        {
            finishGeneration(QString());

            if (!result.success)
            {
                qWarning().noquote() << "[GeminiAI] Interfejs odebrał błąd:"
                                     << result.errorMessage;
                if (statusLabel)
                {
                    setStatusText("status_ai_error");
                }
                QMessageBox::warning(this, LanguageManager::text("gemini_title"), result.errorMessage);
                return;
            }

            currentFilePath.clear();
            applyMachineData(result.machineData);
            qInfo().noquote() << "[GeminiAI] Maszyna zastosowana w edytorze"
                              << "stany=" + QString::number(result.machineData.numStates)
                              << "przejścia=" + QString::number(static_cast<int>(result.machineData.transitions.size()));
            if (rebuildMachineFromEditor() && statusLabel)
            {
                setStatusText("status_ai_generated");
            }

            emit titleChanged(tabTitle());
            QMessageBox::information(this, LanguageManager::text("gemini_title"), LanguageManager::text("gemini_generated_success"));
        },
        [this, finishGeneration]()
        {
            finishGeneration("status_ai_canceled");
        });
}

void MachineEditorTab::applyGeneratedMachineData(const MachineData &data)
{
    currentFilePath.clear();
    applyMachineData(data);
    if (rebuildMachineFromEditor() && statusLabel)
    {
        setStatusText("status_ai_generated");
    }

    emit titleChanged(tabTitle());
}
