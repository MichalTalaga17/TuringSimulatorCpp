#include "ui/GeminiApiKeysDialog.h"

#include "ui/GeminiDialogStyle.h"

#include <QDialogButtonBox>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QtGlobal>

namespace
{
QString freeApiKeyFromSettings(const QSettings &settings)
{
    const QString envKey = qEnvironmentVariable("GEMINI_FREE_API_KEY").trimmed();
    if (!envKey.isEmpty())
    {
        return envKey;
    }

    return settings.value("gemini/freeApiKey").toString().trimmed();
}

QString proApiKeyFromSettings(const QSettings &settings)
{
    const QString envKey = qEnvironmentVariable("GEMINI_PRO_API_KEY").trimmed();
    if (!envKey.isEmpty())
    {
        return envKey;
    }

    const QString legacyEnvKey = qEnvironmentVariable("GEMINI_API_KEY").trimmed();
    if (!legacyEnvKey.isEmpty())
    {
        return legacyEnvKey;
    }

    const QString storedKey = settings.value("gemini/proApiKey").toString().trimmed();
    if (!storedKey.isEmpty())
    {
        return storedKey;
    }

    return settings.value("gemini/apiKey").toString().trimmed();
}
} // koniec przestrzeni nazw

GeminiApiKeysDialog::GeminiApiKeysDialog(
    const QString &initialFreeApiKey,
    const QString &initialProApiKey,
    QWidget *parent)
    : QDialog(parent),
      freeApiKeyField(new QLineEdit(this)),
      proApiKeyField(new QLineEdit(this))
{
    setObjectName("geminiApiKeysDialog");
    setWindowTitle("Ustawienia Gemini API");
    setModal(true);
    setMinimumWidth(560);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel("Klucze Gemini API", this);
    titleLabel->setObjectName("dialogTitle");
    layout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Zapisz osobne klucze dla wariantu Free i wariantu Pro.", this);
    subtitleLabel->setObjectName("dialogSubtitle");
    subtitleLabel->setWordWrap(true);
    layout->addWidget(subtitleLabel);

    auto *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(10);

    freeApiKeyField->setText(initialFreeApiKey);
    freeApiKeyField->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    freeApiKeyField->setPlaceholderText("Klucz API wariantu Free");
    formLayout->addRow("Wariant Free:", freeApiKeyField);

    proApiKeyField->setText(initialProApiKey);
    proApiKeyField->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    proApiKeyField->setPlaceholderText("Klucz API wariantu Pro");
    formLayout->addRow("Wariant Pro:", proApiKeyField);

    layout->addLayout(formLayout);
    layout->addStretch(1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Zapisz");
    buttonBox->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("Anuluj");
    buttonBox->button(QDialogButtonBox::Cancel)->setObjectName("secondaryButton");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    setStyleSheet(geminiDialogStyleSheet());
}

QString GeminiApiKeysDialog::freeApiKey() const
{
    return freeApiKeyField->text().trimmed();
}

QString GeminiApiKeysDialog::proApiKey() const
{
    return proApiKeyField->text().trimmed();
}

QString GeminiApiKeysDialog::apiKeyForTier(const QString &tierId, const QSettings &settings)
{
    return tierId == "free"
               ? freeApiKeyFromSettings(settings)
               : proApiKeyFromSettings(settings);
}

bool GeminiApiKeysDialog::editStoredKeys(QWidget *parent, QSettings &settings)
{
    GeminiApiKeysDialog dialog(
        settings.value("gemini/freeApiKey").toString().trimmed(),
        settings.value("gemini/proApiKey", settings.value("gemini/apiKey")).toString().trimmed(),
        parent);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    settings.setValue("gemini/freeApiKey", dialog.freeApiKey());
    settings.setValue("gemini/proApiKey", dialog.proApiKey());
    qInfo().noquote() << "[GeminiAI] Klucze Gemini API zapisane w QSettings";
    return true;
}
