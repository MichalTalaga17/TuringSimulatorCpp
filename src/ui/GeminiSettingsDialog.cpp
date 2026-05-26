#include "ui/GeminiSettingsDialog.h"

#include "GeminiMachineGenerator.h"
#include "ui/GeminiDialogStyle.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

GeminiSettingsDialog::GeminiSettingsDialog(
    const QString &initialDescription,
    const QString &initialTier,
    bool initialThinking,
    QWidget *parent)
    : QDialog(parent),
      descriptionEdit(new QTextEdit(this)),
      tierCombo(new QComboBox(this)),
      thinkingCheckBox(new QCheckBox("Włącz myślenie modelu", this)),
      buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this))
{
    setObjectName("geminiSettingsDialog");
    setWindowTitle("Generuj maszynę z Gemini");
    setModal(true);
    setMinimumWidth(560);
    resize(620, 440);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel("Generuj maszynę z Gemini", this);
    titleLabel->setObjectName("dialogTitle");
    layout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Opisz zadanie, wybierz wariant API i ustaw tryb myślenia.", this);
    subtitleLabel->setObjectName("dialogSubtitle");
    subtitleLabel->setWordWrap(true);
    layout->addWidget(subtitleLabel);

    auto *descriptionLabel = new QLabel("Opis maszyny", this);
    descriptionLabel->setObjectName("fieldLabel");
    layout->addWidget(descriptionLabel);

    descriptionEdit->setPlainText(initialDescription);
    descriptionEdit->setPlaceholderText("np. Odejmowanie 1 od liczby zapisanej binarnie na taśmie.");
    descriptionEdit->setMinimumHeight(130);
    descriptionEdit->setAcceptRichText(false);
    layout->addWidget(descriptionEdit);

    auto *settingsFrame = new QFrame(this);
    settingsFrame->setObjectName("settingsFrame");
    auto *formLayout = new QFormLayout(settingsFrame);
    formLayout->setContentsMargins(14, 12, 14, 12);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(10);

    const QStringList tiers = GeminiMachineGenerator::availableTierIds();
    const QString selectedInitialTier = tiers.contains(initialTier)
                                            ? initialTier
                                            : tiers.first();

    for (const QString &tier : tiers)
    {
        tierCombo->addItem(GeminiMachineGenerator::tierDisplayName(tier), tier);
    }
    tierCombo->setCurrentIndex(tiers.indexOf(selectedInitialTier));
    tierCombo->setMinimumWidth(260);
    formLayout->addRow("Wariant:", tierCombo);

    thinkingCheckBox->setChecked(initialThinking);
    formLayout->addRow("Myślenie:", thinkingCheckBox);

    layout->addWidget(settingsFrame);
    layout->addStretch(1);

    buttonBox->button(QDialogButtonBox::Ok)->setText("Generuj");
    buttonBox->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("Anuluj");
    buttonBox->button(QDialogButtonBox::Cancel)->setObjectName("secondaryButton");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    connect(descriptionEdit, &QTextEdit::textChanged, this, [this]()
            { updateAcceptButton(); });
    updateAcceptButton();

    setStyleSheet(geminiDialogStyleSheet());
}

QString GeminiSettingsDialog::taskDescription() const
{
    return descriptionEdit->toPlainText().trimmed();
}

QString GeminiSettingsDialog::tierId() const
{
    return tierCombo->currentData().toString();
}

bool GeminiSettingsDialog::thinkingEnabled() const
{
    return thinkingCheckBox->isChecked();
}

void GeminiSettingsDialog::updateAcceptButton()
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!taskDescription().isEmpty());
}
