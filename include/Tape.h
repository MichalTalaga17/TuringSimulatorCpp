#pragma once

#include <map>
#include <string>

/// Reprezentuje ruch głowicy wykonywany po zapisaniu symbolu.
enum class HeadMove {
    L,
    R,
    Stay,
    STOP
};

/// Zwraca tekstową reprezentację ruchu zgodną z notacją pokazywaną użytkownikowi.
std::string moveToString(HeadMove move);

/// Modeluje nieskończoną taśmę przechowywaną w postaci rzadkiej mapy komórek.
class Tape {
public:
    /// Tworzy pustą taśmę z wybranym symbolem pustym.
    explicit Tape(char blankSymbol = '_');

    /// Wczytuje początkową zawartość taśmy i ustawia głowicę na pozycji 0.
    void load(const std::string& input);
    /// Odczytuje symbol spod aktualnej pozycji głowicy.
    char read() const;
    /// Odczytuje symbol z dowolnej pozycji taśmy bez przesuwania głowicy.
    char readAt(int position) const;
    /// Zapisuje symbol pod głowicą. Zapis symbolu pustego usuwa komórkę z mapy.
    void write(char symbol);
    /// Przesuwa głowicę zgodnie z kierunkiem przejścia.
    void move(HeadMove moveDirection);

    /// Zwraca bieżącą pozycję głowicy względem początku taśmy.
    int headPosition() const;
    /// Zwraca symbol traktowany jako pusty.
    char blank() const;
    /// Buduje wycinek taśmy z przedziału domkniętego <from, to>.
    std::string snapshot(int from, int to) const;

private:
    std::map<int, char> cells_;
    int headPosition_ = 0;
    char blankSymbol_;
};
