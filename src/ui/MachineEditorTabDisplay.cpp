#include "MachineEditorTab.h"
#include "ui/TransitionTableRoles.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QSignalBlocker>

#include <algorithm>

namespace
{
constexpr int minimumTapeCellCount = 41;
constexpr int tapeInputTailPadding = 4;
} // koniec przestrzeni nazw

void MachineEditorTab::updateDisplay()
{
    updateStateDisplay();
    updateTapeDisplay();
    updateActiveTransitionHighlight();
}

void MachineEditorTab::updateActiveTransitionHighlight()
{
    if (!machine || !transitionsTable || machine->isHalted())
    {
        clearActiveTransitionHighlight();
        return;
    }

    highlightTransition(machine->currentState(), machine->tape().read());
}

void MachineEditorTab::highlightTransition(const std::string &state, char readSymbol)
{
    if (!transitionsTable)
    {
        return;
    }

    const auto stateIt = std::find(states.begin(), states.end(), state);
    if (stateIt == states.end())
    {
        clearActiveTransitionHighlight();
        return;
    }

    const auto transitionIt = transitionMap.find({state, readSymbol});
    if (transitionIt == transitionMap.end())
    {
        clearActiveTransitionHighlight();
        return;
    }

    const int row = static_cast<int>(std::distance(states.begin(), stateIt));
    int col = 0;
    bool symbolFound = false;
    for (char alphabetSymbol : getOrderedAlphabet())
    {
        if (alphabetSymbol == readSymbol)
        {
            symbolFound = true;
            break;
        }
        ++col;
    }

    if (!symbolFound || row >= transitionsTable->rowCount() || col >= transitionsTable->columnCount())
    {
        clearActiveTransitionHighlight();
        return;
    }

    if (highlightedTransitionRow == row && highlightedTransitionCol == col)
    {
        return;
    }

    clearActiveTransitionHighlight();
    setTransitionCellHighlighted(row, col, true);
    highlightedTransitionRow = row;
    highlightedTransitionCol = col;
    transitionsTable->scrollTo(transitionsTable->model()->index(row, col), QAbstractItemView::PositionAtCenter);
}

void MachineEditorTab::clearActiveTransitionHighlight()
{
    if (highlightedTransitionRow >= 0 && highlightedTransitionCol >= 0)
    {
        setTransitionCellHighlighted(highlightedTransitionRow, highlightedTransitionCol, false);
    }

    highlightedTransitionRow = -1;
    highlightedTransitionCol = -1;
}

void MachineEditorTab::setTransitionCellHighlighted(int row, int col, bool highlighted)
{
    if (!transitionsTable || row < 0 || col < 0)
    {
        return;
    }

    QTableWidgetItem *item = transitionsTable->item(row, col);
    if (!item)
    {
        return;
    }

    QSignalBlocker blockTableSignals(transitionsTable);
    item->setData(ui::ActiveTransitionRole, highlighted);
    transitionsTable->viewport()->update(transitionsTable->visualItemRect(item));
}

void MachineEditorTab::updateStateDisplay()
{
    if (machine)
    {
        machineStateLabel->setText(QString::fromStdString(machine->currentState()));
        stepCountLabel->setText(QString::fromStdString(std::to_string(machine->stepCount())));
    }
}

void MachineEditorTab::updateTapeDisplay()
{
    if (!machine || !tapeScrollArea)
    {
        return;
    }

    ensureTapeCellsFitInput();
    if (tapeCells.empty())
    {
        return;
    }

    const Tape &tape = machine->tape();
    const std::size_t activeCellIndex = tapeCells.size() / 2;
    const int headPos = tape.headPosition();
    const int from = headPos - static_cast<int>(activeCellIndex);

    for (std::size_t index = 0; index < tapeCells.size(); ++index)
    {
        const int tapePosition = from + static_cast<int>(index);
        const char symbol = tape.readAt(tapePosition);
        QLabel *cell = tapeCells[index];

        cell->setText(QString(symbol));
        applyTapeCellStyle(cell, index == activeCellIndex);
    }

    QLabel *activeCell = tapeCells[activeCellIndex];
    const int viewportWidth = tapeScrollArea->viewport()->width();
    if (viewportWidth <= 0)
    {
        scheduleTapeDisplayRefresh();
        return;
    }

    const int targetValue = activeCell->x() + (activeCell->width() / 2) - (viewportWidth / 2);
    tapeScrollArea->horizontalScrollBar()->setValue(std::max(0, targetValue));
}

void MachineEditorTab::scheduleTapeDisplayRefresh()
{
    if (tapeDisplayRefreshTimer->isActive())
    {
        return;
    }

    tapeDisplayRefreshTimer->start(0);
}

void MachineEditorTab::setupTapeCells()
{
    tapeCells.clear();

    auto *layout = qobject_cast<QHBoxLayout *>(tapeDisplay->layout());
    if (!layout)
    {
        return;
    }

    for (int index = 0; index < minimumTapeCellCount; ++index)
    {
        QLabel *cell = createTapeCell();
        layout->addWidget(cell);
        tapeCells.push_back(cell);
    }

    applyTapeCellStyle(tapeCells[tapeCells.size() / 2], true);
}

void MachineEditorTab::ensureTapeCellsFitInput()
{
    if (!tapeDisplay || !tapeInputField)
    {
        return;
    }

    auto *layout = qobject_cast<QHBoxLayout *>(tapeDisplay->layout());
    if (!layout)
    {
        return;
    }

    const int inputLength = tapeInputField->text().trimmed().size();
    const int requiredHalfWidth = std::max(
        minimumTapeCellCount / 2,
        std::max(0, inputLength - 1) + tapeInputTailPadding);
    const int requiredCellCount = (requiredHalfWidth * 2) + 1;

    while (static_cast<int>(tapeCells.size()) < requiredCellCount)
    {
        QLabel *cell = createTapeCell();
        layout->addWidget(cell);
        tapeCells.push_back(cell);
    }

    tapeDisplay->adjustSize();
    tapeDisplay->updateGeometry();
}

QLabel *MachineEditorTab::createTapeCell()
{
    QLabel *cell = new QLabel("-", tapeDisplay);
    cell->setAlignment(Qt::AlignCenter);
    cell->setFixedSize(46, 58);
    applyTapeCellStyle(cell, false);
    return cell;
}

void MachineEditorTab::applyTapeCellStyle(QLabel *cell, bool active)
{
    if (!cell)
    {
        return;
    }

    if (active)
    {
        cell->setStyleSheet(
            "background-color: #eaf4ff;"
            "border: 2px solid #0078d4;"
            "border-radius: 10px;"
            "color: #005a9e;"
            "font-size: 22px;"
            "font-weight: 700;");
        return;
    }

    cell->setStyleSheet(
        "background-color: #ffffff;"
        "border: 1px solid #d0d0d0;"
        "border-radius: 10px;"
        "color: #202020;"
        "font-size: 20px;"
        "font-weight: 600;");
}
