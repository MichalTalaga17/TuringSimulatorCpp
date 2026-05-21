#include "MachineEditorTab.h"
#include "storage.h"
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QInputDialog>

void MachineEditorTab::rebuildTransitionsTable()
{
    transitionsTable->blockSignals(true);
    clearActiveTransitionHighlight();
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

    // Create cells using QTableWidgetItem instead of custom widgets to fix the lag
    for (int row = 0; row < numRows; ++row)
    {
        transitionsTable->setRowHeight(row, 38);
        for (int col = 0; col < numCols; ++col)
        {
            char readSymbol = orderedAlpha[col];
            QString defaultState = QString::fromStdString(states[row]);
            
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            // Default rule: write = read, move = R, state = current
            QString defaultText = QString("%1, R, %2").arg(readSymbol).arg(defaultState);
            item->setText(defaultText);
            
            transitionsTable->setItem(row, col, item);
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

void MachineEditorTab::populateTransitionMapWithDefaults()
{
    transitionMap.clear();

    std::vector<char> orderedAlpha = getOrderedAlphabet();
    int colIdx = 0;
    for (char readSymbol : orderedAlpha)
    {
        for (int row = 0; row < static_cast<int>(states.size()); ++row)
        {
            QTableWidgetItem *item = transitionsTable->item(row, colIdx);
            if (!item) continue;

            QString text = item->text();
            QStringList parts = text.split(", ");
            if (parts.size() == 3)
            {
                transitionMap[{states[row], readSymbol}] = {
                    parts[0][0].toLatin1(),
                    parts[1].toStdString(),
                    parts[2].toStdString()};
            }
        }
        colIdx++;
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
    // ODBLOKUJEMY SYGNAŁY UI
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
            QTableWidgetItem *item = transitionsTable->item(row, col);
            if (item)
            {
                QString transitionText = QString("%1, %2, %3")
                    .arg(writeSymbol)
                    .arg(uiMove == "Stay" ? "L" : uiMove)
                    .arg(QString::fromStdString(nextState));
                item->setText(transitionText);
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

bool MachineEditorTab::saveToDatabase()
{
    if (!rebuildMachineFromEditor()) return false;

    // Zakładka SAMA pyta o nazwę, jeśli to konieczne
    QString machineName = tabTitle();
    bool ok;
    machineName = QInputDialog::getText(this, "Zapisz do bazy", "Podaj nazwę dla tej maszyny:", QLineEdit::Normal, machineName, &ok);

    // Jeśli użytkownik anulował
    if (!ok || machineName.isEmpty()) return false;

    const QString currentInitialState = initialStateCombo->currentText();
    const char currentBlank = blankSymbolField->text().isEmpty() ? '_' : blankSymbolField->text()[0].toLatin1();

    if (!saveMachineToDatabase(*machine, machineName, currentInitialState, currentBlank, states.size(), alphabet)) {
        QMessageBox::warning(this, "Błąd", "Wystąpił problem z zapisem do bazy danych.");
        return false;
    }

    statusLabel->setText("Zapisano w bazie");
    emit titleChanged(machineName); // Aktualizujemy nazwę na karcie
    QMessageBox::information(this, "Sukces", "Zapisano maszynę w bazie danych!");
    return true;
}

bool MachineEditorTab::loadFromDatabase(const QString &machineName)
{
    // 1. Pobieramy surowe dane z bazy danych przez warstwę Storage
    MachineData data = loadMachineFromDatabase(machineName);

    // 2. Jeśli coś poszło nie tak (np. baza nie istnieje), informujemy użytkownika
    if (!data.success) {
        QMessageBox::warning(this, "Błąd", data.errorMessage);
        return false;
    }

    // 3. Używamy Twojej funkcji pomocniczej, która ustawia UI
    // (blokuje sygnały, czyści tabelę, wypełnia ComboBoxy itp.)
    applyMachineData(data);

    // 4. Przebudowujemy logikę maszyny (rebuild), żeby symulator wiedział co uruchomić
    if (rebuildMachineFromEditor() && statusLabel) {
        statusLabel->setText("Wczytano z bazy: " + machineName);
    }

    // 5. Aktualizujemy nazwę karty na tę wczytaną z bazy
    emit titleChanged(machineName);

    return true;
}

// Funkcja pomocnicza odwalająca brudną robotę w interfejsie
void MachineEditorTab::applyMachineData(const MachineData& data)
{
    numStatesSpinBox->blockSignals(true);
    alphabetInputField->blockSignals(true);
    blankSymbolField->blockSignals(true);
    initialStateCombo->blockSignals(true);

    blankSymbolField->setText(QString(QChar(data.blankSymbol)));
    numStatesSpinBox->setValue(data.numStates);

    QString newAlphabetStr = "";
    for (char c : data.alphabet) {
        if (c != data.blankSymbol) newAlphabetStr += c;
    }
    alphabetInputField->setText(newAlphabetStr);

    states.clear();
    for (int i = 0; i < data.numStates; ++i) {
        states.push_back("q" + std::to_string(i));
    }
    alphabet = data.alphabet;

    initialStateCombo->clear();
    for (const auto &state : states) {
        initialStateCombo->addItem(QString::fromStdString(state));
    }
    if (!data.initialState.empty()) {
        initialStateCombo->setCurrentText(QString::fromStdString(data.initialState));
    }

    rebuildTransitionsTable();

    numStatesSpinBox->blockSignals(false);
    alphabetInputField->blockSignals(false);
    blankSymbolField->blockSignals(false);
    initialStateCombo->blockSignals(false);

    for (const auto &transition : data.transitions) {
        std::string currentState = std::get<0>(transition);
        char readSymbol = std::get<1>(transition);
        std::string nextState = std::get<2>(transition);
        char writeSymbol = std::get<3>(transition);
        std::string move = std::get<4>(transition);

        QString uiMove = "Stay";
        if (move == "L") uiMove = "L";
        else if (move == "P" || move == "R") uiMove = "R";
        else if (move == "_") uiMove = "Stay";
        else if (move == "STOP") uiMove = "Stop";

        int row = -1;
        for (size_t s = 0; s < states.size(); ++s) {
            if (states[s] == currentState) { row = static_cast<int>(s); break; }
        }

        int col = -1;
        int colIdx = 0;
        for (char a : alphabet) {
            if (a == readSymbol) { col = colIdx; break; }
            colIdx++;
        }

        if (row != -1 && col != -1) {
            QTableWidgetItem *item = transitionsTable->item(row, col);
            if (item)
            {
                QString transitionText = QString("%1, %2, %3")
                    .arg(writeSymbol)
                    .arg(uiMove)
                    .arg(QString::fromStdString(nextState));
                item->setText(transitionText);
            }
        }
    }
}
