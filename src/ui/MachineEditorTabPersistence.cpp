#include "MachineEditorTab.h"
#include "storage.h"
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QInputDialog>
#include <QDebug>

#include <algorithm>

namespace {

QStringList splitTransitionText(const QString &text)
{
    QStringList parts = text.split(",");
    for (QString &part : parts)
    {
        part = part.trimmed();
    }
    return parts;
}

QString moveToUiText(const std::string &move)
{
    if (move == "L")
    {
        return "L";
    }
    if (move == "P" || move == "R")
    {
        return "R";
    }
    if (move == "STOP" || move == "Stop")
    {
        return "Stop";
    }
    return "Stay";
}

int columnForSymbol(const std::vector<char> &orderedAlphabet, char symbol)
{
    for (int col = 0; col < static_cast<int>(orderedAlphabet.size()); ++col)
    {
        if (orderedAlphabet[col] == symbol)
        {
            return col;
        }
    }
    return -1;
}

} // koniec przestrzeni nazw

void MachineEditorTab::rebuildTransitionsTable()
{
    const bool wasBlocked = transitionsTable->blockSignals(true);
    clearActiveTransitionHighlight();
    transitionsTable->clear();

    if (states.empty() || alphabet.empty())
    {
        transitionMap.clear();
        transitionsTable->blockSignals(wasBlocked);
        return;
    }

    int numRows = static_cast<int>(states.size());
    int numCols = static_cast<int>(alphabet.size());
    transitionsTable->setRowCount(numRows);
    transitionsTable->setColumnCount(numCols);

    // Ustaw nagłówki pionowe z nazwami stanów.
    QStringList stateLabels;
    for (const auto &state : states)
    {
        stateLabels << QString::fromStdString(state);
    }
    transitionsTable->setVerticalHeaderLabels(stateLabels);

    // Ustaw nagłówki poziome z symbolami alfabetu; symbol pusty jest zawsze pierwszy.
    QStringList alphabetLabels;
    std::vector<char> orderedAlpha = getOrderedAlphabet();
    for (char symbol : orderedAlpha)
    {
        alphabetLabels << QString(symbol);
    }
    transitionsTable->setHorizontalHeaderLabels(alphabetLabels);

    // Użyj QTableWidgetItem zamiast własnych widżetów, żeby uniknąć opóźnień.
    for (int row = 0; row < numRows; ++row)
    {
        transitionsTable->setRowHeight(row, 38);
        for (int col = 0; col < numCols; ++col)
        {
            char readSymbol = orderedAlpha[col];
            QString defaultState = QString::fromStdString(states[row]);
            
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            // Reguła domyślna: zapis = odczyt, ruch = R, stan = bieżący.
            QString defaultText = QString("%1, R, %2").arg(readSymbol).arg(defaultState);
            item->setText(defaultText);
            
            transitionsTable->setItem(row, col, item);
        }
    }

    transitionsTable->blockSignals(wasBlocked);

    // Wypełnij mapę przejść wartościami domyślnymi.
    populateTransitionMapWithDefaults();
}

std::vector<char> MachineEditorTab::getOrderedAlphabet() const
{
    // Zwróć alfabet z symbolem pustym zawsze na pierwszej pozycji.
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
            QStringList parts = splitTransitionText(text);
            if (parts.size() == 3 && !parts[0].isEmpty() && !parts[1].isEmpty() && !parts[2].isEmpty())
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

// === ODCZYT I ZAPIS PLIKÓW ===

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

    applyMachineData(data);

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
    currentFilePath.clear();
    untitledTitle = machineName;
    emit titleChanged(tabTitle());
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

    // 3. Używamy funkcji pomocniczej, która ustawia UI
    // (blokuje sygnały, czyści tabelę, wypełnia ComboBoxy itp.)
    applyMachineData(data);

    // 4. Przebudowujemy logikę maszyny, żeby symulator wiedział co uruchomić
    if (rebuildMachineFromEditor() && statusLabel) {
        statusLabel->setText("Wczytano z bazy: " + machineName);
    }

    // 5. Aktualizujemy nazwę karty na tę wczytaną z bazy
    currentFilePath.clear();
    untitledTitle = machineName;
    emit titleChanged(tabTitle());

    return true;
}

// Funkcja pomocnicza stosująca dane maszyny w interfejsie.
void MachineEditorTab::applyMachineData(const MachineData& data)
{
    numStatesSpinBox->blockSignals(true);
    alphabetInputField->blockSignals(true);
    blankSymbolField->blockSignals(true);
    initialStateCombo->blockSignals(true);
    transitionsTable->blockSignals(true);

    blankSymbol = data.blankSymbol;
    previousBlankSymbol = data.blankSymbol;
    const int stateCount = std::max(1, data.numStates);

    blankSymbolField->setText(QString(QChar(blankSymbol)));
    numStatesSpinBox->setValue(stateCount);

    QString newAlphabetStr = "";
    for (char c : data.alphabet) {
        if (c != blankSymbol) newAlphabetStr += c;
    }
    alphabetInputField->setText(newAlphabetStr);

    states.clear();
    for (int i = 0; i < stateCount; ++i) {
        states.push_back("q" + std::to_string(i));
    }

    alphabet.clear();
    alphabet.push_back(blankSymbol);
    for (char symbol : data.alphabet)
    {
        if (symbol != blankSymbol &&
            std::find(alphabet.begin(), alphabet.end(), symbol) == alphabet.end())
        {
            alphabet.push_back(symbol);
        }
    }

    initialStateCombo->clear();
    for (const auto &state : states) {
        initialStateCombo->addItem(QString::fromStdString(state));
    }
    initialState = data.initialState;
    if (std::find(states.begin(), states.end(), initialState) == states.end() && !states.empty()) {
        initialState = states.front();
    }
    if (!initialState.empty()) {
        initialStateCombo->setCurrentText(QString::fromStdString(initialState));
    }

    rebuildTransitionsTable();
    const std::vector<char> orderedAlpha = getOrderedAlphabet();

    for (const auto &transition : data.transitions) {
        std::string currentState = std::get<0>(transition);
        char readSymbol = std::get<1>(transition);
        std::string nextState = std::get<2>(transition);
        char writeSymbol = std::get<3>(transition);
        std::string move = std::get<4>(transition);

        int row = -1;
        for (int r = 0; r < transitionsTable->rowCount(); ++r) {
            if (transitionsTable->verticalHeaderItem(r)->text().toStdString() == currentState) {
                row = r; break;
            }
        }

        int col = columnForSymbol(orderedAlpha, readSymbol);

        if (row != -1 && col != -1) {
            QTableWidgetItem *item = transitionsTable->item(row, col);
            if (item)
            {
                QString transitionText = QString("%1, %2, %3")
                    .arg(writeSymbol)
                    .arg(moveToUiText(move))
                    .arg(QString::fromStdString(nextState));
                item->setText(transitionText);
            }
        }
    }

    populateTransitionMapWithDefaults();

    transitionsTable->blockSignals(false);
    numStatesSpinBox->blockSignals(false);
    alphabetInputField->blockSignals(false);
    blankSymbolField->blockSignals(false);
    initialStateCombo->blockSignals(false);
}
