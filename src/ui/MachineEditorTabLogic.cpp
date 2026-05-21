#include "MachineEditorTab.h"

#include <QAbstractItemView>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QSignalBlocker>

#include <algorithm>

namespace
{
    constexpr int minimumTapeCellCount = 41;
    constexpr int tapeInputTailPadding = 4;
} // namespace

QString MachineEditorTab::tabTitle() const
{
    if (!currentFilePath.isEmpty())
    {
        return QFileInfo(currentFilePath).fileName();
    }

    return untitledTitle;
}

bool MachineEditorTab::validateMachine()
{
    const bool isValid = buildMachineFromCurrentConfiguration(true);
    if (isValid && machine)
    {
        machine->loadTape(tapeInputField->text().trimmed().toStdString());
        updateDisplay();
    }
    return isValid;
}

void MachineEditorTab::resetToDefaultExample()
{
    currentFilePath.clear();
    applyDefaultExample();
    emit titleChanged(tabTitle());
}

void MachineEditorTab::onNumStatesChanged(int value)
{
    states.clear();
    for (int i = 0; i < value; ++i)
    {
        states.push_back("q" + std::to_string(i));
    }

    // Update combos with signal blocking
    initialStateCombo->blockSignals(true);
    initialStateCombo->clear();

    for (const auto &state : states)
    {
        initialStateCombo->addItem(QString::fromStdString(state));
    }

    if (initialStateCombo->count() > 0)
    {
        initialStateCombo->setCurrentIndex(0);
        initialState = states[0];
    }
    initialStateCombo->blockSignals(false);

    // Completely rebuild table because states logic needs new items
    rebuildTransitionsTable();

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel)
    {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void MachineEditorTab::onAlphabetChanged()
{
    QString alphaStr = alphabetInputField->text();
    std::set<char> newAlphabet;

    // Get current blank symbol
    QString blankStr = blankSymbolField->text();
    char currentBlank = blankStr.isEmpty() || blankStr.length() != 1 ? '_' : blankStr[0].toLatin1();

    QString filteredAlphaStr;
    bool modified = false;

    // Parse alphabet (comma or space separated, or just chars)
    for (int i = 0; i < alphaStr.length(); ++i)
    {
        char c = alphaStr[i].toLatin1();
        if (c == currentBlank)
        {
            modified = true;
            continue;
        }
        filteredAlphaStr += alphaStr[i];

        if (c != ',' && c != ' ' && c != 0)
        {
            newAlphabet.insert(c);
        }
    }

    if (modified)
    {
        alphabetInputField->blockSignals(true);
        int cursor = alphabetInputField->cursorPosition();
        alphabetInputField->setText(filteredAlphaStr);
        alphabetInputField->setCursorPosition(std::max(0, cursor - 1));
        alphabetInputField->blockSignals(false);
    }

    // Always add the blank symbol as first column (if not already in alphabet)
    newAlphabet.insert(currentBlank);

    // Only rebuild if alphabet changed
    if (newAlphabet != alphabet)
    {
        alphabet = newAlphabet;
        rebuildTransitionsTable();
        // Note: transitionMap is populated by rebuildTransitionsTable() via populateTransitionMapWithDefaults()
    }

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel)
    {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void MachineEditorTab::onBlankSymbolChanged()
{
    QString blankStr = blankSymbolField->text();
    char newBlank = blankStr.isEmpty() || blankStr.length() != 1 ? '_' : blankStr[0].toLatin1();

    // If blank symbol hasn't changed, do nothing
    if (newBlank == blankSymbol)
    {
        return;
    }

    // Remove the new blank symbol from the alphabet text field
    QString alphaStr = alphabetInputField->text();
    bool alphabetModified = false;
    if (alphaStr.contains(QChar(newBlank)))
    {
        alphabetInputField->blockSignals(true);
        alphaStr.remove(QChar(newBlank));
        alphabetInputField->setText(alphaStr);
        alphabetInputField->blockSignals(false);
        alphabetModified = true;
    }

    // Store the old blank symbol before changing it
    char oldBlank = blankSymbol;

    // Update the blank symbol immediately
    previousBlankSymbol = blankSymbol;
    blankSymbol = newBlank;

    // Update the alphabet set
    std::set<char> newAlphabet = alphabet;
    newAlphabet.erase(oldBlank);
    newAlphabet.insert(newBlank);

    if (alphabetModified)
    {
        alphabet = newAlphabet;
        rebuildTransitionsTable();
    }
    else 
    {
        alphabet = newAlphabet;
        // If only the blank symbol character changed without affecting the rest of the alphabet, update in place
        if (transitionsTable->rowCount() > 0 && transitionsTable->columnCount() > 0)
        {
            // The blank symbol should always be at column 0 due to getOrderedAlphabet()
            transitionsTable->horizontalHeaderItem(0)->setText(QString(newBlank));
        }
    }

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel)
    {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void MachineEditorTab::onTableItemChanged(QTableWidgetItem *item)
{
    if (!item) return;

    int row = item->row();
    int col = item->column();

    if (row < 0 || col < 0 || row >= static_cast<int>(states.size())) return;

    QString text = item->text();
    QStringList parts = text.split(",");

    std::string state = states[row];
    std::vector<char> orderedAlpha = getOrderedAlphabet();
    
    if (col >= static_cast<int>(orderedAlpha.size())) return;
    char symbol = orderedAlpha[col];

    for (QString &part : parts)
    {
        part = part.trimmed();
    }

    if (parts.size() == 3 && !parts[0].isEmpty() && !parts[1].isEmpty() && !parts[2].isEmpty())
    {
        QString writeStr = parts[0].trimmed();
        char writeC = writeStr.isEmpty() ? '_' : writeStr[0].toLatin1();
        transitionMap[{state, symbol}] = {writeC, parts[1].trimmed().toStdString(), parts[2].trimmed().toStdString()};
    }
    else
    {
        transitionMap.erase({state, symbol});
    }

    if (rebuildMachineFromEditor() && statusLabel)
    {
        statusLabel->setText("Rebuilt");
    }
}

bool MachineEditorTab::buildMachineFromCurrentConfiguration(bool showSuccessDialog)
{
    auto showValidationWarning = [this, showSuccessDialog](const QString &message)
    {
        if (showSuccessDialog)
        {
            QMessageBox::warning(this, "Validation Error", message);
        }
    };

    if (initialStateCombo && !initialStateCombo->currentText().isEmpty())
    {
        initialState = initialStateCombo->currentText().toStdString();
    }

    QString blankStr = blankSymbolField->text();
    if (blankStr.isEmpty() || blankStr.length() != 1)
    {
        blankSymbol = '_';
    }
    else
    {
        blankSymbol = blankStr[0].toLatin1();
    }
    alphabet.insert(blankSymbol);

    // Validation
    if (states.empty())
    {
        showValidationWarning("Please set the number of states");
        return false;
    }

    if (alphabet.empty())
    {
        showValidationWarning("Please define the alphabet");
        return false;
    }

    if (initialState.empty())
    {
        showValidationWarning("Please select an initial state");
        return false;
    }

    if (transitionMap.empty())
    {
        showValidationWarning("Please define at least one transition");
        return false;
    }

    // Build machine
    machine = std::make_unique<TuringMachine>(blankSymbol);
    machine->setInitialState(initialState);

    // Add transitions from map
    for (const auto &[key, value] : transitionMap)
    {
        std::string fromState = key.first;
        char readSymbol = key.second;

        char writeSymbol = std::get<0>(value);
        std::string moveStr = std::get<1>(value);
        std::string toState = std::get<2>(value);

        HeadMove move = HeadMove::Stay;
        if (moveStr == "L")
            move = HeadMove::L;
        else if (moveStr == "R" || moveStr == "P")
            move = HeadMove::R;
        else if (moveStr == "Stop" || moveStr == "STOP")
            move = HeadMove::STOP;

        machine->addTransition(fromState, readSymbol, {writeSymbol, move, toState});
    }

    if (showSuccessDialog)
    {
        statusLabel->setText("Built");
        QMessageBox::information(this, "Success", "Machine built successfully! You can run it above.");
    }

    return true;
}

bool MachineEditorTab::executeMachineStep()
{
    if (!machine)
    {
        return false;
    }

    const bool executedStep = machine->step();
    updateDisplay();

    if (!executedStep || machine->isHalted())
    {
        isRunning = false;
        runButton->setText("Run");
        stepButton->setEnabled(true);
        executionTimer->stop();
        statusLabel->setText(machine->isAccepted() ? "Accepted" : "Halted");
        return false;
    }

    return true;
}

bool MachineEditorTab::rebuildMachineFromEditor()
{
    if (isRunning)
    {
        isRunning = false;
        executionTimer->stop();
        runButton->setText("Run");
        stepButton->setEnabled(true);
    }

    if (!buildMachineFromCurrentConfiguration(false) || !machine)
    {
        return false;
    }

    machine->loadTape(tapeInputField->text().trimmed().toStdString());
    updateDisplay();
    return true;
}

// === Executor Slots ===

void MachineEditorTab::onTapeInputChanged()
{
    if (tapeWarningLabel) {
        tapeWarningLabel->hide();
    }

    if (rebuildMachineFromEditor() && statusLabel)
    {
        statusLabel->setText("Rebuilt");
    }
}

void MachineEditorTab::checkTapeForUndefinedSymbols()
{
    QString tapeText = tapeInputField->text().trimmed();
    bool hasUndefined = false;
    for (int i = 0; i < tapeText.length(); ++i) {
        char c = tapeText[i].toLatin1();
        if (alphabet.find(c) == alphabet.end() && c != blankSymbol) {
            hasUndefined = true;
            break;
        }
    }

    if (tapeWarningLabel) {
        tapeWarningLabel->setVisible(hasUndefined);
    }
}

void MachineEditorTab::onStepClicked()
{
    if (!machine)
    {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    checkTapeForUndefinedSymbols();

    executeMachineStep();
}

void MachineEditorTab::onRunClicked()
{
    if (!machine)
    {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    if (!isRunning)
    {
        checkTapeForUndefinedSymbols();
    }

    isRunning = !isRunning;
    runButton->setText(isRunning ? "Stop" : "Run");
    stepButton->setEnabled(!isRunning);

    if (isRunning)
    {
        statusLabel->setText("Running");
        if (executeMachineStep())
        {
            executionTimer->start(2001 - executionSpeed);
        }
    }
    else
    {
        statusLabel->setText("Paused");
        executionTimer->stop();
    }
}

void MachineEditorTab::onResetClicked()
{
    if (tapeWarningLabel) {
        tapeWarningLabel->hide();
    }

    if (machine)
    {
        machine->reset();
        isRunning = false;
        runButton->setText("Run");
        stepButton->setEnabled(true);
        executionTimer->stop();
        updateDisplay();
        statusLabel->setText("Reset");
    }
}

void MachineEditorTab::onSpeedChanged(int value)
{
    executionSpeed = value;
    if (isRunning)
    {
        executionTimer->setInterval(2001 - value);
    }
}

void MachineEditorTab::onExecutionTimer()
{
    if (machine && isRunning)
    {
        executeMachineStep();
    }
}

// === Display Updates ===

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

    const std::string &currentState = machine->currentState();
    const auto stateIt = std::find(states.begin(), states.end(), currentState);
    if (stateIt == states.end())
    {
        clearActiveTransitionHighlight();
        return;
    }

    const char readSymbol = machine->tape().read();
    const auto transitionIt = transitionMap.find({currentState, readSymbol});
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
    if (highlighted)
    {
        // Pomaluj delikatnie tło na niebiesko aby wskazać wykonywany krok 
        // a przy zgaszeniu cofnij by zajął się tym menedżer motywu
        item->setBackground(QColor(63, 116, 216, 80));
    }
    else
    {
        item->setBackground(QBrush());
    }
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
        const char symbol = tape.snapshot(tapePosition, tapePosition).front();
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

void MachineEditorTab::applyDefaultExample()
{
    numStatesSpinBox->setValue(1);
    alphabetInputField->setText("01");
    blankSymbolField->setText("_");
    tapeInputField->setText("100010111");

    auto findColumnForSymbol = [this](char symbol) -> int
    {
        int columnIndex = 0;
        std::vector<char> orderedAlpha = getOrderedAlphabet();
        for (char alphabetSymbol : orderedAlpha)
        {
            if (alphabetSymbol == symbol)
            {
                return columnIndex;
            }
            ++columnIndex;
        }
        return -1;
    };

    const int row = 0;
    const int blankColumn = findColumnForSymbol('_');
    const int zeroColumn = findColumnForSymbol('0');
    const int oneColumn = findColumnForSymbol('1');

    auto setTransitionCell = [this, row](int column, char writeSymbol, const QString &moveText, const QString &nextStateText)
    {
        if (column < 0)
        {
            return;
        }

        QTableWidgetItem *item = transitionsTable->item(row, column);
        if (item)
        {
            item->setText(QString("%1, %2, %3").arg(writeSymbol).arg(moveText).arg(nextStateText));
        }
    };

    setTransitionCell(zeroColumn, '1', "R", "q0");
    setTransitionCell(oneColumn, '0', "R", "q0");
    setTransitionCell(blankColumn, '_', "Stop", "q0");

    populateTransitionMapWithDefaults();

    if (rebuildMachineFromEditor())
    {
        statusLabel->setText("Ready");
    }
}

void MachineEditorTab::applyBlankMachine()
{
    numStatesSpinBox->setValue(2);
    alphabetInputField->setText("01");
    blankSymbolField->setText("_");
    tapeInputField->clear();

    populateTransitionMapWithDefaults();

    if (rebuildMachineFromEditor())
    {
        statusLabel->setText("Ready");
    }
}
