#pragma once

/**
 * @file GeminiGenerationRunner.h
 * @brief Wspólny przepływ UI dla generowania maszyny przez Gemini.
 */

#include "GeminiMachineGenerator.h"

#include <QString>

#include <functional>

class QWidget;

struct GeminiGenerationRequest
{
    QString description;
    QString apiKey;
    QString modelName;
    QString fallbackModelName;
    bool thinkingEnabled = false;
};

void runGeminiGenerationWithProgress(
    QWidget *parent,
    const GeminiGenerationRequest &request,
    std::function<void(const GeminiMachineGenerator::Result &)> onFinished,
    std::function<void()> onCanceled = {});
