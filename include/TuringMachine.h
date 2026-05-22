#pragma once

/**
 * @file TuringMachine.h
 * @brief Rdzeń deterministycznej jednotaśmowej maszyny Turinga.
 */

#include "Tape.h"

#include <map>
#include <optional>
#include <set>
#include <string>

/**
 * @brief Opisuje pojedyncze przejście funkcji przejścia maszyny Turinga.
 */
struct Transition
{
    /// Symbol zapisywany w bieżącej komórce taśmy.
    char writeSymbol;
    /// Ruch głowicy wykonywany po zapisie symbolu.
    HeadMove move;
    /// Stan sterowania ustawiany po wykonaniu przejścia.
    std::string nextState;
};

/**
 * @brief Udostępnia logikę jednotaśmowej deterministycznej maszyny Turinga.
 *
 * Klasa przechowuje stan sterowania, taśmę, zbiór stanów akceptujących oraz
 * deterministyczną funkcję przejścia. Warstwa UI buduje tę klasę z formularza,
 * a potem wykonuje symulację przez kolejne wywołania step().
 */
class TuringMachine
{
public:
    /**
     * @brief Tworzy maszynę z wybranym symbolem pustym taśmy.
     * @param blankSymbol Symbol pustej komórki używany przez wewnętrzną taśmę.
     */
    explicit TuringMachine(char blankSymbol = '_');

    /**
     * @brief Ustawia stan, od którego zaczyna się każde uruchomienie lub reset.
     * @param state Nazwa stanu początkowego.
     */
    void setInitialState(std::string state);
    /**
     * @brief Rejestruje stan kończący obliczenia z wynikiem akceptacji.
     * @param state Nazwa stanu akceptującego.
     */
    void addAcceptingState(std::string state);
    /**
     * @brief Dodaje regułę dla pary: aktualny stan i odczytany symbol.
     * @param state Stan, w którym reguła ma zostać użyta.
     * @param readSymbol Symbol wymagany pod głowicą.
     * @param transition Operacja zapisu, ruchu i zmiany stanu.
     */
    void addTransition(
        const std::string &state,
        char readSymbol,
        Transition transition);
    /**
     * @brief Wczytuje wejściową zawartość taśmy oraz zapamiętuje ją na potrzeby resetu.
     * @param input Początkowy ciąg symboli taśmy.
     */
    void loadTape(const std::string &input);
    /**
     * @brief Przywraca stan początkowy, licznik kroków i startową zawartość taśmy.
     */
    void reset();

    /**
     * @brief Wykonuje jeden krok obliczeń.
     * @return `true`, gdy wykonano przejście; `false`, gdy maszyna była już zatrzymana
     * albo nie znaleziono instrukcji dla bieżącego stanu i symbolu.
     */
    bool step();
    /**
     * @brief Informuje, czy maszyna nie może już wykonać kolejnego kroku.
     * @return `true`, jeśli maszyna jest zatrzymana.
     */
    bool isHalted() const;
    /**
     * @brief Informuje, czy zatrzymanie nastąpiło w stanie akceptującym.
     * @return `true`, jeśli maszyna zatrzymała się w stanie akceptującym.
     */
    bool isAccepted() const;

    /**
     * @brief Zwraca nazwę aktualnego stanu sterowania.
     * @return Referencja do nazwy bieżącego stanu.
     */
    const std::string &currentState() const;
    /**
     * @brief Udostępnia bieżący stan taśmy tylko do odczytu.
     * @return Referencja do taśmy maszyny.
     */
    const Tape &tape() const;
    /**
     * @brief Zwraca liczbę wykonanych kroków od ostatniego resetu.
     * @return Licznik poprawnie wykonanych przejść.
     */
    std::size_t stepCount() const;
    /**
     * @brief Mapa przejść indeksowana parą `(stan, odczytany symbol)`.
     */
    using TransitionMap = std::map<std::pair<std::string, char>, Transition>;
    /**
     * @brief Udostępnia zdefiniowane przejścia do zapisu w pliku lub bazie.
     * @return Niemodyfikowalna mapa przejść.
     */
    const TransitionMap &getTransitions() const { return transitions_; };

private:
    /// Klucz mapy przejść: aktualny stan i symbol odczytany z taśmy.
    using TransitionKey = std::pair<std::string, char>;

    /// Stan ustawiany podczas resetu maszyny.
    std::string initialState_;
    /// Aktualny stan sterowania.
    std::string currentState_;
    /// Taśma robocza maszyny.
    Tape tape_;
    /// Deterministyczna funkcja przejścia.
    std::map<TransitionKey, Transition> transitions_;
    /// Stany zatrzymujące obliczenia z wynikiem akceptacji.
    std::set<std::string> acceptingStates_;
    /// Wejście używane do odtworzenia taśmy podczas resetu.
    std::optional<std::string> initialTapeInput_;
    /// Liczba przejść wykonanych od ostatniego resetu.
    std::size_t stepCount_ = 0;
    /// Flaga oznaczająca, że maszyna nie wykona kolejnego kroku.
    bool halted_ = false;

    /**
     * @brief Aktualizuje stan zatrzymania na podstawie bieżącego stanu sterowania.
     */
    void updateHaltedForCurrentState();
};
