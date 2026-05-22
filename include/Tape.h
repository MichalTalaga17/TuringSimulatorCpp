#pragma once

/**
 * @file Tape.h
 * @brief Model rzadko zapisanej, nieskończonej taśmy maszyny Turinga.
 */

#include <map>
#include <string>

/**
 * @brief Reprezentuje ruch głowicy wykonywany po zapisaniu symbolu.
 */
enum class HeadMove {
    /// Przesunięcie głowicy o jedną komórkę w lewo.
    L,
    /// Przesunięcie głowicy o jedną komórkę w prawo.
    R,
    /// Pozostawienie głowicy na bieżącej komórce.
    Stay,
    /// Zatrzymanie obliczeń po wykonaniu przejścia.
    STOP
};

/**
 * @brief Zwraca tekstową reprezentację ruchu zgodną z notacją pokazywaną użytkownikowi.
 *
 * @param move Ruch głowicy do zapisania.
 * @return Napis używany w UI i plikach zapisu: `L`, `P`, `_` albo `STOP`.
 * @throws std::runtime_error Gdy przekazano nieobsługiwaną wartość enumeracji.
 */
std::string moveToString(HeadMove move);

/**
 * @brief Modeluje nieskończoną taśmę przechowywaną jako rzadka mapa komórek.
 *
 * Taśma zapisuje tylko komórki różne od symbolu pustego. Dzięki temu puste
 * obszary po lewej i prawej stronie wejścia nie zajmują pamięci, a odczyt
 * nieustawionej pozycji zawsze zwraca `blankSymbol_`.
 */
class Tape {
public:
    /**
     * @brief Tworzy pustą taśmę z wybranym symbolem pustym.
     * @param blankSymbol Symbol traktowany jako pusta komórka.
     */
    explicit Tape(char blankSymbol = '_');

    /**
     * @brief Wczytuje początkową zawartość taśmy i ustawia głowicę na pozycji 0.
     * @param input Ciąg symboli zapisywany od pozycji 0 w prawo.
     */
    void load(const std::string& input);
    /**
     * @brief Odczytuje symbol spod aktualnej pozycji głowicy.
     * @return Symbol zapisany w bieżącej komórce albo symbol pusty.
     */
    char read() const;
    /**
     * @brief Odczytuje symbol z dowolnej pozycji taśmy bez przesuwania głowicy.
     * @param position Pozycja komórki względem początku wejścia.
     * @return Symbol zapisany w komórce albo symbol pusty.
     */
    char readAt(int position) const;
    /**
     * @brief Zapisuje symbol pod głowicą.
     *
     * Zapis symbolu pustego usuwa komórkę z mapy, aby zachować rzadką
     * reprezentację taśmy.
     *
     * @param symbol Symbol do zapisania w bieżącej komórce.
     */
    void write(char symbol);
    /**
     * @brief Przesuwa głowicę zgodnie z kierunkiem przejścia.
     * @param moveDirection Kierunek ruchu głowicy.
     * @throws std::runtime_error Gdy przekazano nieobsługiwaną wartość ruchu.
     */
    void move(HeadMove moveDirection);

    /**
     * @brief Zwraca bieżącą pozycję głowicy względem początku taśmy.
     * @return Indeks aktualnej komórki głowicy.
     */
    int headPosition() const;
    /**
     * @brief Zwraca symbol traktowany jako pusty.
     * @return Symbol pustej komórki.
     */
    char blank() const;
    /**
     * @brief Buduje wycinek taśmy z przedziału domkniętego `<from, to>`.
     * @param from Pierwsza pozycja wycinka.
     * @param to Ostatnia pozycja wycinka.
     * @return Napis reprezentujący kolejne komórki wycinka.
     */
    std::string snapshot(int from, int to) const;

private:
    /// Niepuste komórki taśmy indeksowane pozycją.
    std::map<int, char> cells_;
    /// Aktualna pozycja głowicy względem początku wejścia.
    int headPosition_ = 0;
    /// Symbol zwracany dla pustych lub usuniętych komórek.
    char blankSymbol_;
};
