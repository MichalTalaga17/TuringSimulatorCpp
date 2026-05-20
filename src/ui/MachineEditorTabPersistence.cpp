#include "MachineEditorTab.h"
#include "storage.h"

#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>

void MachineEditorTab::rebuildTransitionsTable()
{
    transitionsTable->blockSignals(true);
    clearActiveTransitionHighlight();
    clearTableCache();
    transitionsTable->clear();

    if (states.empty() || alphabet.empty())
    {
        transitionsTable->blockSignals(false);
        return;
    }

    int numRows = static_cast<int>(states.size());
    int numCols = static_cast<int>(alphabet.size());
    transitionsTable->setRowCount(numRows);
    transitionsTable->setColumnCount(numCols);

    // Set vertical headers (state names)
    QStringList stateLabels;
    for (const auto &state : states)
    {
        stateLabels << QString::fromStdString(state);
    }
    transitionsTable->setVerticalHeaderLabels(stateLabels);

    // Set horizontal headers (alphabet symbols) - with blank symbol always first
    QStringList alphabetLabels;
    std::vector<char> orderedAlpha = getOrderedAlphabet();
    for (char symbol : orderedAlpha)
    {
        alphabetLabels << QString(symbol);
    }
    transitionsTable->setHorizontalHeaderLabels(alphabetLabels);

    // Create cells with embedded combo boxes
    for (int row = 0; row < numRows; ++row)
    {
        transitionsTable->setRowHeight(row, 38);
        for (int col = 0; col < numCols; ++col)
        {
            transitionsTable->setCellWidget(row, col, createTransitionCell(row, col));
        }
    }

    transitionsTable->blockSignals(false);

    // Populate transition map with default values
    populateTransitionMapWithDefaults();
}

std::vector<char> MachineEditorTab::getOrderedAlphabet() const
{
    // Return alphabet with blank symbol always first
    std::vector<char> result;
    result.push_back(blankSymbol);
    for (char c : alphabet)
    {
        if (c != blankSymbol)
        {
            result.push_back(c);
        }
    }
    return result;
}

void MachineEditorTab::updateAllWriteCombosForBlankSymbolChange(char oldBlank, char newBlank)
{
    // Iterate through all cells in the table and update write combos that have oldBlank selected
    for (int row = 0; row < transitionsTable->rowCount(); ++row)
    {
        for (int col = 0; col < transitionsTable->columnCount(); ++col)
        {
            QWidget *cellWidget = transitionsTable->cellWidget(row, col);
            if (!cellWidget)
                continue;

            QComboBox *writeCombo = cellWidget->findChild<QComboBox *>(QString("write_%1_%2").arg(row).arg(col));
            if (writeCombo)
            {
                // Check if this combo has the old blank symbol selected
                if (writeCombo->currentText() == QString(oldBlank))
                {
                    writeCombo->blockSignals(true);
                    // Find and select the new blank symbol
                    int newIdx = writeCombo->findText(QString(newBlank));
                    if (newIdx >= 0)
                    {
                        writeCombo->setCurrentIndex(newIdx);
                    }
                    writeCombo->blockSignals(false);
                }
            }
        }
    }
}

void MachineEditorTab::populateTransitionMapWithDefaults()
{
    // Populate the transition map with default values for all cells
    // Defaults: write = read symbol, move = L, state = current state

    transitionMap.clear();

    std::vector<char> orderedAlpha = getOrderedAlphabet();
    int colIdx = 0;
    for (char readSymbol : orderedAlpha)
    {
        for (int row = 0; row < static_cast<int>(states.size()); ++row)
        {
            QWidget *cellWidget = transitionsTable->cellWidget(row, colIdx);
            if (!cellWidget)
                continue;

            QComboBox *writeCombo = cellWidget->findChild<QComboBox *>(QString("write_%1_%2").arg(row).arg(colIdx));
            QComboBox *moveCombo = cellWidget->findChild<QComboBox *>(QString("move_%1_%2").arg(row).arg(colIdx));
            QComboBox *toStateCombo = cellWidget->findChild<QComboBox *>(QString("tostate_%1_%2").arg(row).arg(colIdx));

            if (writeCombo && moveCombo && toStateCombo)
            {
                QString write = writeCombo->currentText();
                QString move = moveCombo->currentText();
                QString toState = toStateCombo->currentText();

                if (!write.isEmpty() && !move.isEmpty() && !toState.isEmpty())
                {
                    transitionMap[{states[row], readSymbol}] = {
                        write[0].toLatin1(),
                        move.toStdString(),
                        toState.toStdString()};
                }
            }
        }
        colIdx++;
    }
}

void MachineEditorTab::clearTableCache()
{
    comboCache.clear();
}

void MachineEditorTab::setCellDefaults(int row, int col, QComboBox *writeCombo, QComboBox *moveCombo, QComboBox *toStateCombo)
{
    if (!writeCombo || !moveCombo || !toStateCombo)
        return;

    // Block signals during default setting
    writeCombo->blockSignals(true);
    moveCombo->blockSignals(true);
    toStateCombo->blockSignals(true);

    // Get the read symbol for this column using ordered alphabet
    char readSymbol = '0';
    std::vector<char> orderedAlpha = getOrderedAlphabet();
    if (col < static_cast<int>(orderedAlpha.size()))
    {
        readSymbol = orderedAlpha[col];
    }

    // Set write combo to read symbol (default: write = read)
    int writeIdx = writeCombo->findText(QString(readSymbol));
    if (writeIdx >= 0)
    {
        writeCombo->setCurrentIndex(writeIdx);
    }

    // Set move combo to "L" (default: always left)
    int moveIdx = moveCombo->findText("L");
    if (moveIdx >= 0)
    {
        moveCombo->setCurrentIndex(moveIdx);
    }

    // Set state combo to current state (default: stay in same state)
    if (row < static_cast<int>(states.size()))
    {
        int stateIdx = toStateCombo->findText(QString::fromStdString(states[row]));
        if (stateIdx >= 0)
        {
            toStateCombo->setCurrentIndex(stateIdx);
        }
    }

    // Unblock signals
    writeCombo->blockSignals(false);
    moveCombo->blockSignals(false);
    toStateCombo->blockSignals(false);
}

void MachineEditorTab::changeColumnSymbol(int col, char oldSymbol, char newSymbol)
{
    // Find the column index for the old blank symbol
    int colIdx = 0;
    for (char s : alphabet)
    {
        if (s == oldSymbol && colIdx == col)
        {
            // Update the horizontal header label
            transitionsTable->horizontalHeaderItem(col)->setText(QString(newSymbol));

            // Update all write combos in this column
            updateWriteComboForColumn(col, newSymbol);

            return;
        }
        colIdx++;
    }
}

void MachineEditorTab::updateWriteComboForColumn(int col, char newSymbol)
{
    // Iterate through all rows and update the write combo for this column
    std::vector<char> orderedAlpha = getOrderedAlphabet();

    for (int row = 0; row < transitionsTable->rowCount(); ++row)
    {
        QWidget *cellWidget = transitionsTable->cellWidget(row, col);
        if (!cellWidget)
            continue;

        QComboBox *writeCombo = cellWidget->findChild<QComboBox *>(QString("write_%1_%2").arg(row).arg(col));
        if (writeCombo)
        {
            writeCombo->blockSignals(true);

            // Store current index
            int currentIdx = writeCombo->currentIndex();
            QString currentText = writeCombo->currentText();

            // Clear and rebuild the write combo with updated alphabet
            writeCombo->clear();
            for (char c : orderedAlpha)
            {
                writeCombo->addItem(QString(c));
            }

            // Restore the selection if the old symbol isn't the new blank
            // or select the new blank symbol if it was selected before
            int newIdx = -1;
            if (currentText == QString(newSymbol))
            {
                // Old selection was the old blank symbol, update to new blank
                newIdx = writeCombo->findText(QString(newSymbol));
            }
            else
            {
                // Keep the same text if it still exists
                newIdx = writeCombo->findText(currentText);
                if (newIdx < 0 && currentIdx >= 0 && currentIdx < writeCombo->count())
                {
                    newIdx = currentIdx;
                }
            }

            if (newIdx >= 0)
            {
                writeCombo->setCurrentIndex(newIdx);
            }

            writeCombo->blockSignals(false);
        }
    }
}

// === FILE I/O ===

bool MachineEditorTab::saveToFile()
{
    if (currentFilePath.isEmpty())
    {
        return saveToFileAs();
    }

    return saveMachineToPath(currentFilePath);
}

bool MachineEditorTab::saveToFileAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Zapisz maszynę", currentFilePath, "JSON Files (*.json)");
    if (filePath.isEmpty())
    {
        return false;
    }

    return saveMachineToPath(filePath);
}

bool MachineEditorTab::saveMachineToPath(const QString &filePath)
{
    if (!rebuildMachineFromEditor())
    {
        return false;
    }

    const QString currentInitialState = initialStateCombo->currentText();
    const char currentBlank = blankSymbolField->text().isEmpty() ? '_' : blankSymbolField->text()[0].toLatin1();

    // --- TUTAJ JEST ZMIANA - DODAJEMY states.size() i alphabet ---
    if (!saveInstructionsToFile(*machine, filePath, currentInitialState, currentBlank, states.size(), alphabet))
    {
        QMessageBox::warning(this, "Błąd", "Nie udało się zapisać pliku.");
        return false;
    }

    currentFilePath = filePath;
    statusLabel->setText("Zapisano maszynę");
    emit titleChanged(tabTitle());
    QMessageBox::information(this, "Sukces", "Zapisano pomyślnie do pliku.");
    return true;
}

bool MachineEditorTab::loadFromFile(const QString &filePath)
{
    MachineData data = loadInstructionsFromFile(filePath);
    if (!data.success)
    {
        QMessageBox::warning(this, "Błąd", data.errorMessage);
        return false;
    }

    // ==========================================================
    // BLOKUJEMY SYGNAŁY UI
    // ==========================================================
    numStatesSpinBox->blockSignals(true);
    alphabetInputField->blockSignals(true);
    blankSymbolField->blockSignals(true);
    initialStateCombo->blockSignals(true);

    blankSymbolField->setText(QString(QChar(data.blankSymbol)));
    numStatesSpinBox->setValue(data.numStates);

    QString newAlphabetStr = "";
    for (char c : data.alphabet)
    {
        if (c != data.blankSymbol)
        {
            newAlphabetStr += c;
        }
    }
    alphabetInputField->setText(newAlphabetStr);

    states.clear();
    for (int i = 0; i < data.numStates; ++i)
    {
        states.push_back("q" + std::to_string(i));
    }
    alphabet = data.alphabet;

    initialStateCombo->clear();
    for (const auto &state : states)
    {
        initialStateCombo->addItem(QString::fromStdString(state));
    }
    if (!data.initialState.empty())
    {
        initialStateCombo->setCurrentText(QString::fromStdString(data.initialState));
    }

    // 4. Budujemy idealnie dopasowaną tabelę JEDEN RAZ
    rebuildTransitionsTable();

    // ==========================================================
    // ODBLOKOWUJEMY SYGNAŁY UI
    // ==========================================================
    numStatesSpinBox->blockSignals(false);
    alphabetInputField->blockSignals(false);
    blankSymbolField->blockSignals(false);
    initialStateCombo->blockSignals(false);

    // 5. Uzupełniamy komórki gotowej tabeli
    for (const auto &transition : data.transitions)
    {
        std::string currentState = std::get<0>(transition);
        char readSymbol = std::get<1>(transition);
        std::string nextState = std::get<2>(transition);
        char writeSymbol = std::get<3>(transition);
        std::string move = std::get<4>(transition);

        QString uiMove = "Stay";
        if (move == "L")
            uiMove = "L";
        else if (move == "P" || move == "R")
            uiMove = "R";
        else if (move == "_")
            uiMove = "Stay";
        else if (move == "STOP")
            uiMove = "Stop";

        int row = -1;
        for (size_t s = 0; s < states.size(); ++s)
        {
            if (states[s] == currentState)
            {
                row = static_cast<int>(s);
                break;
            }
        }

        int col = -1;
        int colIdx = 0;
        for (char a : alphabet)
        {
            if (a == readSymbol)
            {
                col = colIdx;
                break;
            }
            colIdx++;
        }

        if (row != -1 && col != -1)
        {
            auto cacheIt = comboCache.find({row, col});
            if (cacheIt != comboCache.end())
            {
                std::get<0>(cacheIt->second)->setCurrentText(QString(writeSymbol));
                std::get<1>(cacheIt->second)->setCurrentText(uiMove);
                std::get<2>(cacheIt->second)->setCurrentText(QString::fromStdString(nextState));
            }
        }
    }

    if (rebuildMachineFromEditor() && statusLabel)
    {
        statusLabel->setText("Wczytano z pliku");
    }

    currentFilePath = filePath;
    emit titleChanged(tabTitle());
    return true;
}
