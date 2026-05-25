#include "MachineEditorTab.h"

#include "GeminiMachineGenerator.h"
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
        qInfo().noquote() << "[GeminiAI] Canceled: generation dialog rejected";
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
        qInfo().noquote() << "[GeminiAI] Canceled: empty description";
        return false;
    }
    if (tierId.isEmpty() || modelName->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Canceled: tier not selected";
        return false;
    }

    *apiKey = GeminiApiKeysDialog::apiKeyForTier(tierId, settings);
    if (apiKey->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] API key source:"
                          << "settings dialog required";
        if (!GeminiApiKeysDialog::editStoredKeys(parent, settings))
        {
            qInfo().noquote() << "[GeminiAI] Canceled: API keys not configured";
            return false;
        }

        *apiKey = GeminiApiKeysDialog::apiKeyForTier(tierId, settings);
    }
    else
    {
        qInfo().noquote() << "[GeminiAI] API key source:"
                          << (tierId == "free" ? "Free tier" : "Pro tier");
    }

    if (apiKey->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Canceled: selected tier API key missing";
        QMessageBox::warning(
            parent,
            "Gemini API",
            QString("Brakuje klucza API dla %1. Uzupełnij go w ustawieniach Gemini.")
                .arg(GeminiMachineGenerator::tierDisplayName(tierId)));
        return false;
    }

    settings.setValue("gemini/selectedTier", tierId);
    settings.setValue("gemini/thinkingEnabled", *thinkingEnabled);
    qInfo().noquote() << "[GeminiAI] Description accepted"
                      << "description=\"" + shortLogText(*description) + "\"";
    qInfo().noquote() << "[GeminiAI] Tier selected:" << tierConfig.displayName;
    qInfo().noquote() << "[GeminiAI] Model selected:" << *modelName;
    if (!fallbackModelName->isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Fallback model:" << *fallbackModelName;
    }
    qInfo().noquote() << "[GeminiAI] Thinking selected:"
                      << (*thinkingEnabled ? "on" : "off");
    return true;
}

void MachineEditorTab::generateMachineWithAi()
{
    qInfo().noquote() << "[GeminiAI] UI action triggered";

    if (aiGenerationInProgress)
    {
        qInfo().noquote() << "[GeminiAI] Ignored: generation already in progress";
        QMessageBox::information(this, "Gemini AI", "Generowanie maszyny jest już w toku.");
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
        qInfo().noquote() << "[GeminiAI] Ignored: generation already in progress";
        QMessageBox::information(this, "Gemini AI", "Generowanie maszyny jest już w toku.");
        return;
    }

    const QString trimmedDescription = description.trimmed();
    const QString trimmedApiKey = apiKey.trimmed();
    const QString selectedModel = modelName.trimmed();
    const QString selectedFallbackModel = fallbackModelName.trimmed();

    if (trimmedDescription.isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Canceled: empty description";
        return;
    }

    if (trimmedApiKey.isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Canceled: empty API key";
        return;
    }

    if (selectedModel.isEmpty())
    {
        qInfo().noquote() << "[GeminiAI] Canceled: empty model";
        return;
    }

    qInfo().noquote() << "[GeminiAI] Starting generation in editor"
                      << "model=" + selectedModel
                      << "thinking=" + QString(thinkingEnabled ? "on" : "off")
                      << "description=\"" + shortLogText(trimmedDescription) + "\"";

    aiGenerationInProgress = true;
    if (aiGenerateButton)
    {
        aiGenerateButton->setEnabled(false);
    }
    if (statusLabel)
    {
        statusLabel->setText("AI...");
    }

    auto finishGeneration = [this](const QString &statusText)
    {
        aiGenerationInProgress = false;
        if (aiGenerateButton)
        {
            aiGenerateButton->setEnabled(true);
        }
        if (statusLabel && !statusText.isEmpty())
        {
            statusLabel->setText(statusText);
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
                qWarning().noquote() << "[GeminiAI] UI received failure:"
                                     << result.errorMessage;
                if (statusLabel)
                {
                    statusLabel->setText("Błąd AI");
                }
                QMessageBox::warning(this, "Gemini AI", result.errorMessage);
                return;
            }

            currentFilePath.clear();
            applyMachineData(result.machineData);
            qInfo().noquote() << "[GeminiAI] Machine applied to editor"
                              << "states=" + QString::number(result.machineData.numStates)
                              << "transitions=" + QString::number(static_cast<int>(result.machineData.transitions.size()));
            if (rebuildMachineFromEditor() && statusLabel)
            {
                statusLabel->setText("Wygenerowano AI");
            }

            emit titleChanged(tabTitle());
            QMessageBox::information(this, "Gemini AI", "Maszyna została wygenerowana i wczytana do edytora.");
        },
        [this, finishGeneration]()
        {
            finishGeneration("Anulowano AI");
        });
}

void MachineEditorTab::applyGeneratedMachineData(const MachineData &data)
{
    currentFilePath.clear();
    applyMachineData(data);
    if (rebuildMachineFromEditor() && statusLabel)
    {
        statusLabel->setText("Wygenerowano AI");
    }

    emit titleChanged(tabTitle());
}
