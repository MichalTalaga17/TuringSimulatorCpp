#pragma once

/**
 * @file TransitionDelegate.h
 * @brief Delegat Qt do edycji komórek tabeli przejść maszyny.
 */

#include <QStyledItemDelegate>

class MachineEditorTab;

/**
 * @brief Rysuje i edytuje pojedyncze instrukcje przejścia w QTableWidget.
 *
 * Delegat tworzy kompaktowy edytor z trzema listami: symbol zapisu, ruch
 * głowicy i następny stan. Obsługuje też wizualne podświetlenie aktywnej
 * instrukcji wykonywanej przez symulator.
 */
class TransitionDelegate : public QStyledItemDelegate
{
public:
    /**
     * @brief Tworzy delegata powiązanego z kartą edytora.
     * @param tab Karta dostarczająca alfabet i listę stanów.
     * @param parent Opcjonalny rodzic Qt.
     */
    explicit TransitionDelegate(MachineEditorTab *tab, QObject *parent = nullptr);

    /**
     * @brief Rysuje komórkę tabeli oraz ramkę aktywnego przejścia.
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    /**
     * @brief Tworzy edytor z listami wyboru dla przejścia.
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    /**
     * @brief Przenosi tekst z modelu do pól edytora.
     */
    void setEditorData(QWidget *widget, const QModelIndex &index) const override;
    /**
     * @brief Zapisuje wartości edytora z powrotem do modelu tabeli.
     */
    void setModelData(QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const override;
    /**
     * @brief Dopasowuje geometrię edytora do prostokąta komórki.
     */
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    /// Karta edytora udostępniająca aktualny alfabet i listę stanów.
    MachineEditorTab *m_tab;
};
