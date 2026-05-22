#pragma once

#include "Tape.h"

#include <map>
#include <optional>
#include <set>
#include <string>

/// Opisuje pojedyncze przejście funkcji przejścia maszyny Turinga.
struct Transition
{
    char writeSymbol;
    HeadMove move;
    std::string nextState;
};

/// Udostępnia logikę jednotaśmowej deterministycznej maszyny Turinga.
class TuringMachine
{
public:
    /// Tworzy maszynę z wybranym symbolem pustym taśmy.
    explicit TuringMachine(char blankSymbol = '_');

    /// Ustawia stan, od którego zaczyna się każde uruchomienie lub reset.
    void setInitialState(std::string state);
    /// Rejestruje stan kończący obliczenia z wynikiem akceptacji.
    void addAcceptingState(std::string state);
    /// Dodaje regułę dla pary: aktualny stan i odczytany symbol.
    void addTransition(
        const std::string &state,
        char readSymbol,
        Transition transition);
    /// Wczytuje wejściową zawartość taśmy oraz zapamiętuje ją na potrzeby resetu.
    void loadTape(const std::string &input);
    /// Przywraca stan początkowy, licznik kroków i startową zawartość taśmy.
    void reset();

    /// Wykonuje jeden krok obliczeń. Zwraca `false`, jeśli maszyna była lub stała się zatrzymana.
    bool step();
    /// Informuje, czy maszyna nie może już wykonać kolejnego kroku.
    bool isHalted() const;
    /// Informuje, czy zatrzymanie nastąpiło w stanie akceptującym.
    bool isAccepted() const;

    /// Zwraca nazwę aktualnego stanu sterowania.
    const std::string &currentState() const;
    /// Udostępnia bieżący stan taśmy tylko do odczytu.
    const Tape &tape() const;
    /// Zwraca liczbę wykonanych kroków od ostatniego resetu.
    std::size_t stepCount() const;
    /// Getter do zapisu instrukcji z maszyny
    using TransitionMap = std::map<std::pair<std::string, char>, Transition>;
    const TransitionMap &getTransitions() const { return transitions_; };

private:
    using TransitionKey = std::pair<std::string, char>;

    std::string initialState_;
    std::string currentState_;
    Tape tape_;
    std::map<TransitionKey, Transition> transitions_;
    std::set<std::string> acceptingStates_;
    std::optional<std::string> initialTapeInput_;
    std::size_t stepCount_ = 0;
    bool halted_ = false;

    void updateHaltedForCurrentState();
};
