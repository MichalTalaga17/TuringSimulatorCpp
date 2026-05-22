#pragma once

#include <QStyledItemDelegate>

class MachineEditorTab;

class TransitionDelegate : public QStyledItemDelegate
{
public:
    explicit TransitionDelegate(MachineEditorTab *tab, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *widget, const QModelIndex &index) const override;
    void setModelData(QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    MachineEditorTab *m_tab;
};
