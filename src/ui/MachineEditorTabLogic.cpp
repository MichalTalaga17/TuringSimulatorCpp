#include "MachineEditorTab.h"

#include <QAbstractItemView>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>

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

    // Update only the toState combos if table already exists with same column count
    if (transitionsTable->rowCount() > 0 && transitionsTable->columnCount() > 0)
    {
        transitionsTable->blockSignals(true);

        int oldRowCount = transitionsTable->rowCount();
        int newRowCount = static_cast<int>(states.size());

        // Set row count FIRST
        transitionsTable->setRowCount(newRowCount);

        // THEN update vertical headers (state names) after row count is set
        QStringList stateLabels;
        for (const auto &state : states)
        {
            stateLabels << QString::fromStdString(state);
        }
        transitionsTable->setVerticalHeaderLabels(stateLabels);

        // Update ALL existing rows with new state list (this is key for fixing the dropdown issue)
        for (int row = 0; row < oldRowCount && row < newRowCount; ++row)
        {
            transitionsTable->setRowHeight(row, 38);
            for (int col = 0; col < transitionsTable->columnCount(); ++col)
            {
                QWidget *cellWidget = transitionsTable->cellWidget(row, col);
                if (!cellWidget)
                    continue;

                QComboBox *toStateCombo = cellWidget->findChild<QComboBox *>(QString("tostate_%1_%2").arg(row).arg(col));
                if (toStateCombo)
                {
                    toStateCombo->blockSignals(true);
                    int currentIdx = toStateCombo->currentIndex();
                    toStateCombo->clear();
                    for (const auto &state : states)
                    {
                        toStateCombo->addItem(QString::fromStdString(state));
                    }
                    if (currentIdx >= 0 && currentIdx < toStateCombo->count())
                    {
                        toStateCombo->setCurrentIndex(currentIdx);
                    }
                    toStateCombo->blockSignals(false);
                }
            }
        }

        // If rows were added, create cells for new rows
        if (newRowCount > oldRowCount)
        {
            for (int row = oldRowCount; row < newRowCount; ++row)
            {
                transitionsTable->setRowHeight(row, 38);
                for (int col = 0; col < transitionsTable->columnCount(); ++col)
                {
                    transitionsTable->setCellWidget(row, col, createTransitionCell(row, col));
                }
            }
        }

        transitionsTable->blockSignals(false);

        // Repopulate transition map with updated defaults
        populateTransitionMapWithDefaults();
    }
    else
    {
        // First time: rebuild the entire table
        rebuildTransitionsTable();
    }

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

    // Parse alphabet (comma or space separated, or just chars)
    for (int i = 0; i < alphaStr.length(); ++i)
    {
        char c = alphaStr[i].toLatin1();
        if (c != ',' && c != ' ' && c != 0)
        {
            newAlphabet.insert(c);
        }
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

    // Store the old blank symbol before changing it
    char oldBlank = blankSymbol;

    // Update the blank symbol immediately
    previousBlankSymbol = blankSymbol;
    blankSymbol = newBlank;

    // If the table exists and has columns, update the blank symbol column in place
    if (transitionsTable->rowCount() > 0 && transitionsTable->columnCount() > 0)
    {
        // The blank symbol should always be at column 0 due to getOrderedAlphabet()
        // Just update the header and write combos
        transitionsTable->horizontalHeaderItem(0)->setText(QString(newBlank));
        updateWriteComboForColumn(0, newBlank);

        // Update all write combos throughout the table that have the old blank symbol selected
        updateAllWriteCombosForBlankSymbolChange(oldBlank, newBlank);
    }

    // Update the alphabet set
    std::set<char> newAlphabet = alphabet;
    newAlphabet.erase(oldBlank);
    newAlphabet.insert(newBlank);
    alphabet = newAlphabet;

    const bool rebuilt = rebuildMachineFromEditor();
    if (statusLabel)
    {
        statusLabel->setText(rebuilt ? "Rebuilt" : "Edit");
    }
}

void MachineEditorTab::onCellComboChanged()
{
    // Sender is a QComboBox inside a cell
    QComboBox *senderCombo = qobject_cast<QComboBox *>(sender());
    if (!senderCombo)
        return;

    // Use the object name to directly determine row/col instead of searching
    QString objName = senderCombo->objectName();
    int row = -1, col = -1;

    // Parse name like "write_0_1" or "move_0_1" or "tostate_0_1"
    QStringList parts = objName.split('_');
    if (parts.size() >= 3)
    {
        row = parts[1].toInt();
        col = parts[2].toInt();
    }

    if (row < 0 || col < 0 || row >= transitionsTable->rowCount() || col >= transitionsTable->columnCount())
    {
        return;
    }

    QWidget *cellWidget = transitionsTable->cellWidget(row, col);
    if (!cellWidget)
        return;

    // Get the 3 combos from cache or find them
    auto cacheKey = std::make_pair(row, col);
    QComboBox *writeCombo = nullptr, *moveCombo = nullptr, *toStateCombo = nullptr;

    auto it = comboCache.find(cacheKey);
    if (it != comboCache.end())
    {
        std::tie(writeCombo, moveCombo, toStateCombo) = it->second;
    }
    else
    {
        writeCombo = cellWidget->findChild<QComboBox *>(QString("write_%1_%2").arg(row).arg(col));
        moveCombo = cellWidget->findChild<QComboBox *>(QString("move_%1_%2").arg(row).arg(col));
        toStateCombo = cellWidget->findChild<QComboBox *>(QString("tostate_%1_%2").arg(row).arg(col));

        if (writeCombo && moveCombo && toStateCombo)
        {
            comboCache[cacheKey] = std::make_tuple(writeCombo, moveCombo, toStateCombo);
        }
    }

    if (!writeCombo || !moveCombo || !toStateCombo)
        return;

    // Build transition string
    QString write = writeCombo->currentText();
    QString move = moveCombo->currentText();
    QString toState = toStateCombo->currentText();

    if (!write.isEmpty() && !move.isEmpty() && !toState.isEmpty())
    {
        // Get the symbol for this column by index using ordered alphabet
        std::string state = states[row];
        char symbol = '0';
        std::vector<char> orderedAlpha = getOrderedAlphabet();
        if (col < static_cast<int>(orderedAlpha.size()))
        {
            symbol = orderedAlpha[col];
        }

        // Store in map
        transitionMap[{state, symbol}] = {write[0].toLatin1(), move.toStdString(), toState.toStdString()};
    }
    else
    {
        // Remove from map if incomplete
        if (row < static_cast<int>(states.size()) && col < static_cast<int>(alphabet.size()))
        {
            std::string state = states[row];
            char symbol = '0';
            std::vector<char> orderedAlpha = getOrderedAlphabet();
            if (col < static_cast<int>(orderedAlpha.size()))
            {
                symbol = orderedAlpha[col];
            }
            transitionMap.erase({state, symbol});
        }
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

    // Extract blank symbol
    QString blankStr = blankSymbolField->text();
    if (blankStr.isEmpty() || blankStr.length() != 1)
    {
        blankSymbol = '_';
    }
    else
    {
        blankSymbol = blankStr[0].toLatin1();
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
        else if (moveStr == "R")
            move = HeadMove::R;
        else if (moveStr == "Stop")
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
    if (rebuildMachineFromEditor() && statusLabel)
    {
        statusLabel->setText("Rebuilt");
    }
}

void MachineEditorTab::onStepClicked()
{
    if (!machine)
    {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    if (machine->step())
    {
        updateDisplay();
    }
    else
    {
        updateDisplay();
        statusLabel->setText(machine->isAccepted() ? "Accepted" : "Halted");
    }
}

void MachineEditorTab::onRunClicked()
{
    if (!machine)
    {
        QMessageBox::warning(this, "Error", "Please build a machine first");
        return;
    }

    isRunning = !isRunning;
    runButton->setText(isRunning ? "Stop" : "Run");
    stepButton->setEnabled(!isRunning);

    if (isRunning)
    {
        statusLabel->setText("Running");
        executionTimer->start(2001 - executionSpeed);
    }
    else
    {
        statusLabel->setText("Paused");
        executionTimer->stop();
    }
}

void MachineEditorTab::onResetClicked()
{
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
        if (!machine->step())
        {
            isRunning = false;
            runButton->setText("Run");
            stepButton->setEnabled(true);
            executionTimer->stop();
            statusLabel->setText(machine->isAccepted() ? "Accepted" : "Halted");
        }
        updateDisplay();
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
    for (char alphabetSymbol : alphabet)
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

    QWidget *cellWidget = transitionsTable->cellWidget(row, col);
    if (!cellWidget)
    {
        return;
    }

    const QString selector = QString("#%1").arg(cellWidget->objectName());
    if (highlighted)
    {
        cellWidget->setStyleSheet(
            selector +
            " {"
            "background-color: #eaf2ff;"
            "border: 1px solid #3f74d8;"
            "border-radius: 6px;"
            "}");
    }
    else
    {
        cellWidget->setStyleSheet(
            selector +
            " {"
            "background-color: transparent;"
            "border: 1px solid #cfcfcf;"
            "border-radius: 6px;"
            "}");
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

        const auto cacheIt = comboCache.find({row, column});
        if (cacheIt == comboCache.end())
        {
            return;
        }

        QComboBox *writeCombo = std::get<0>(cacheIt->second);
        QComboBox *moveCombo = std::get<1>(cacheIt->second);
        QComboBox *toStateCombo = std::get<2>(cacheIt->second);

        writeCombo->setCurrentText(QString(writeSymbol));
        moveCombo->setCurrentText(moveText);
        toStateCombo->setCurrentText(nextStateText);
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
