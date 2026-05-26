#include "ui/GeminiGenerationRunner.h"

#include <QDebug>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QProgressDialog>

#include <utility>

namespace
{
class ProgressGeminiGeneration : public QObject
{
public:
    ProgressGeminiGeneration(
        QWidget *parent,
        GeminiGenerationRequest request,
        std::function<void(const GeminiMachineGenerator::Result &)> onFinished,
        std::function<void()> onCanceled)
        : QObject(parent),
          request(std::move(request)),
          onFinished(std::move(onFinished)),
          onCanceled(std::move(onCanceled)),
          generator(new GeminiMachineGenerator(this)),
          progressDialog(new QProgressDialog("Gemini generuje maszynę...", "Anuluj", 0, 0, parent))
    {
        progressDialog->setAttribute(Qt::WA_DeleteOnClose);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);

        connect(progressDialog, &QProgressDialog::canceled, progressDialog, [this]()
                {
            if (closingProgressDialog)
            {
                return;
            }

            userCanceled = true;
            qInfo().noquote() << "[GeminiAI] Użytkownik zażądał anulowania";
            if (currentReply)
            {
                currentReply->abort();
            } });
    }

    void start()
    {
        progressDialog->show();
        startRequest(request.modelName);
    }

private:
    void startRequest(const QString &modelName)
    {
        if (fallbackUsed && progressDialog)
        {
            progressDialog->setLabelText("Gemini generuje maszynę przez model zapasowy...");
        }

        currentReply = generator->generate(
            request.description,
            request.apiKey,
            modelName,
            request.thinkingEnabled,
            [this](const GeminiMachineGenerator::Result &result)
            {
                handleResult(result);
            });
    }

    void handleResult(const GeminiMachineGenerator::Result &result)
    {
        if (userCanceled)
        {
            closeProgressDialog();
            qInfo().noquote() << "[GeminiAI] Użytkownik anulował żądanie";
            if (onCanceled)
            {
                onCanceled();
            }
            deleteLater();
            return;
        }

        if (!result.success &&
            !fallbackUsed &&
            !request.fallbackModelName.isEmpty() &&
            GeminiMachineGenerator::shouldTryFallback(result))
        {
            fallbackUsed = true;
            qWarning().noquote() << "[GeminiAI] Główny model wariantu Free nie zadziałał, ponawiam przez model zapasowy:"
                                 << request.fallbackModelName
                                 << "powód=\"" + result.errorMessage + "\"";
            startRequest(request.fallbackModelName);
            return;
        }

        closeProgressDialog();

        if (onFinished)
        {
            onFinished(result);
        }

        deleteLater();
    }

    void closeProgressDialog()
    {
        if (progressDialog)
        {
            closingProgressDialog = true;
            progressDialog->close();
        }
    }

    GeminiGenerationRequest request;
    std::function<void(const GeminiMachineGenerator::Result &)> onFinished;
    std::function<void()> onCanceled;
    GeminiMachineGenerator *generator;
    QPointer<QProgressDialog> progressDialog;
    QPointer<QNetworkReply> currentReply;
    bool userCanceled = false;
    bool closingProgressDialog = false;
    bool fallbackUsed = false;
};
} // koniec przestrzeni nazw

void runGeminiGenerationWithProgress(
    QWidget *parent,
    const GeminiGenerationRequest &request,
    std::function<void(const GeminiMachineGenerator::Result &)> onFinished,
    std::function<void()> onCanceled)
{
    auto *generation = new ProgressGeminiGeneration(
        parent,
        request,
        std::move(onFinished),
        std::move(onCanceled));
    generation->start();
}
