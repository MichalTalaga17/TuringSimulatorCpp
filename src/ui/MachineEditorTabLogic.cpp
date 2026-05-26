#include "MachineEditorTab.h"

#include <QFileInfo>
#include <QMessageBox>

#include <algorithm>

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

    // Aktualizuj listy wyboru przy zablokowanych sygnałach.
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

    // Przebuduj całą tabelę, bo zmiana stanów wymaga nowych elementów.
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
    std::vector<char> newAlphabet;

    QString blankStr = blankSymbolField->text();
    char currentBlank = blankStr.isEmpty() || blankStr.length() != 1 ? '_' : blankStr[0].toLatin1();
    newAlphabet.push_back(currentBlank);

    QString filteredAlphaStr;
    bool modified = false;

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
            if (std::find(newAlphabet.begin(), newAlphabet.end(), c) == newAlphabet.end()) {
                newAlphabet.push_back(c);
            }
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

    if (newAlphabet != alphabet)
    {
        alphabet = newAlphabet;
        rebuildTransitionsTable();
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

    if (newBlank == blankSymbol)
    {
        return;
    }

    QString alphaStr = alphabetInputField->text();
    if (alphaStr.contains(QChar(newBlank)))
    {
        alphabetInputField->blockSignals(true);
        alphaStr.remove(QChar(newBlank));
        alphabetInputField->setText(alphaStr);
        alphabetInputField->blockSignals(false);
    }

    char oldBlank = blankSymbol;
    blankSymbol = newBlank;

    std::vector<char> newAlphabet;
    newAlphabet.push_back(newBlank);
    for (char symbol : alphabet)
    {
        if (symbol != oldBlank && symbol != newBlank &&
            std::find(newAlphabet.begin(), newAlphabet.end(), symbol) == newAlphabet.end())
        {
            newAlphabet.push_back(symbol);
        }
    }
    alphabet = newAlphabet;
    rebuildTransitionsTable();

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
    alphabet.erase(std::remove(alphabet.begin(), alphabet.end(), blankSymbol), alphabet.end());
    alphabet.insert(alphabet.begin(), blankSymbol);

    // Walidacja.
    if (states.empty())
    {
        showValidationWarning("Please set the number of states");
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

    // Budowanie maszyny.
    machine = std::make_unique<TuringMachine>(blankSymbol);
    machine->setInitialState(initialState);

    // Dodaj przejścia z mapy.
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

    const std::string executedState = machine->currentState();
    const char executedSymbol = machine->tape().read();
    const bool executedStep = machine->step();
    updateStateDisplay();
    updateTapeDisplay();

    if (executedStep)
    {
        highlightTransition(executedState, executedSymbol);
    }
    else
    {
        clearActiveTransitionHighlight();
    }

    if (!executedStep || machine->isHalted())
    {
        isRunning = false;
        runButton->setText("Run");
        stepButton->setEnabled(true);
        executionTimer->stop();
        statusLabel->setText("Halted");
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

// === SLOTY PANELU WYKONANIA ===

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
        if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end() && c != blankSymbol) {
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
