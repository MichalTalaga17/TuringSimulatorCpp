#include "TransitionDelegate.h"

#include "LanguageManager.h"
#include "MachineEditorTab.h"
#include "ui/TransitionTableRoles.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QListView>
#include <QPainter>
#include <QSignalBlocker>

namespace
{
class TransitionEditor : public QWidget
{
public:
    explicit TransitionEditor(QWidget *parent = nullptr) : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        writeCombo = createComboBox(50);
        moveCombo = createComboBox(65);
        stateCombo = createComboBox(70);

        layout->addWidget(writeCombo);
        layout->addWidget(moveCombo);
        layout->addWidget(stateCombo);
        setFocusPolicy(Qt::StrongFocus);
        setFocusProxy(writeCombo);
        setAutoFillBackground(true);
    }

    QComboBox *writeCombo;
    QComboBox *moveCombo;
    QComboBox *stateCombo;

private:
    QComboBox *createComboBox(int popupMinWidth)
    {
        QComboBox *combo = new QComboBox(this);
        combo->setFixedHeight(26);
        combo->setProperty("isCompact", true);
        combo->setMaxVisibleItems(8);

        auto *view = new QListView(combo);
        view->setMinimumWidth(popupMinWidth);
        combo->setView(view);
        return combo;
    }
};

QStringList splitTransitionText(const QString &text)
{
    QStringList parts = text.split(",");
    for (QString &part : parts)
    {
        part = part.trimmed();
    }
    return parts;
}

QString normalizedMoveText(QString moveText)
{
    moveText = moveText.trimmed();
    return LanguageManager::localizedMoveText(moveText);
}
} // koniec przestrzeni nazw

TransitionDelegate::TransitionDelegate(MachineEditorTab *tab, QObject *parent)
    : QStyledItemDelegate(parent), m_tab(tab)
{
}

void TransitionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if (!index.data(ui::ActiveTransitionRole).toBool())
    {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF highlightRect = option.rect.adjusted(4, 4, -4, -4);
    painter->setBrush(QColor(63, 116, 216, 70));
    painter->setPen(QPen(QColor(63, 116, 216), 2));
    painter->drawRoundedRect(highlightRect, 6, 6);

    painter->restore();
}

QWidget *TransitionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
    TransitionEditor *editor = new TransitionEditor(parent);

    for (char symbol : m_tab->getOrderedAlphabet())
    {
        editor->writeCombo->addItem(QString(symbol));
    }

    editor->moveCombo->addItems({
        LanguageManager::moveLeftText(),
        LanguageManager::moveRightText(),
        LanguageManager::moveStayText(),
        LanguageManager::moveStopText()});

    for (const auto &state : m_tab->getStatesList())
    {
        editor->stateCombo->addItem(QString::fromStdString(state));
    }

    auto commitChanges = [this, editor]()
    {
        auto *self = const_cast<TransitionDelegate *>(this);
        emit self->commitData(editor);
    };

    connect(editor->writeCombo, QOverload<int>::of(&QComboBox::activated), editor, commitChanges);
    connect(editor->moveCombo, QOverload<int>::of(&QComboBox::activated), editor, commitChanges);
    connect(editor->stateCombo, QOverload<int>::of(&QComboBox::activated), editor, commitChanges);

    return editor;
}

void TransitionDelegate::setEditorData(QWidget *widget, const QModelIndex &index) const
{
    if (!widget)
    {
        return;
    }

    auto *editor = static_cast<TransitionEditor *>(widget);
    QSignalBlocker blockWrite(editor->writeCombo);
    QSignalBlocker blockMove(editor->moveCombo);
    QSignalBlocker blockState(editor->stateCombo);

    const QStringList parts = splitTransitionText(index.data(Qt::DisplayRole).toString());
    if (parts.size() == 3)
    {
        editor->writeCombo->setCurrentText(parts[0]);
        editor->moveCombo->setCurrentText(normalizedMoveText(parts[1]));
        editor->stateCombo->setCurrentText(parts[2]);
    }
}

void TransitionDelegate::setModelData(QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (!widget)
    {
        return;
    }

    auto *editor = static_cast<TransitionEditor *>(widget);
    const QString text = QString("%1, %2, %3")
                             .arg(editor->writeCombo->currentText())
                             .arg(editor->moveCombo->currentText())
                             .arg(editor->stateCombo->currentText());

    model->setData(index, text, Qt::DisplayRole);
}

void TransitionDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry(option.rect);
}
