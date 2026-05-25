#include "TuringMachine.h"

#include <utility>

namespace {

bool isAcceptingState(
    const std::set<std::string>& acceptingStates,
    const std::string& currentState
) {
    return acceptingStates.find(currentState) != acceptingStates.end();
}

} // koniec przestrzeni nazw

TuringMachine::TuringMachine(char blankSymbol) : tape_(blankSymbol) {}

void TuringMachine::setInitialState(std::string state) {
    initialState_ = std::move(state);
    currentState_ = initialState_;
    stepCount_ = 0;
    updateHaltedForCurrentState();
}

void TuringMachine::addAcceptingState(std::string state) {
    const bool acceptsCurrentState = state == currentState_;
    acceptingStates_.insert(std::move(state));
    if (acceptsCurrentState) {
        halted_ = true;
    }
}

void TuringMachine::addTransition(
    const std::string& state,
    char readSymbol,
    Transition transition
) {
    transitions_[{state, readSymbol}] = std::move(transition);
}

void TuringMachine::loadTape(const std::string& input) {
    initialTapeInput_ = input;
    reset();
}

void TuringMachine::reset() {
    currentState_ = initialState_;
    stepCount_ = 0;
    tape_.load(initialTapeInput_.value_or(""));
    updateHaltedForCurrentState();
}

bool TuringMachine::step() {
    if (halted_) {
        return false;
    }

    const char readSymbol = tape_.read();
    const auto transitionIt = transitions_.find({currentState_, readSymbol});

    if (transitionIt == transitions_.end()) {
        halted_ = true;
        return false;
    }

    const Transition& transition = transitionIt->second;
    tape_.write(transition.writeSymbol);
    tape_.move(transition.move);
    currentState_ = transition.nextState;
    ++stepCount_;

    if (transition.move == HeadMove::STOP) {
        halted_ = true;
    } else {
        updateHaltedForCurrentState();
    }

    return true;
}

bool TuringMachine::isHalted() const {
    return halted_;
}

bool TuringMachine::isAccepted() const {
    return halted_ && isAcceptingState(acceptingStates_, currentState_);
}

const std::string& TuringMachine::currentState() const {
    return currentState_;
}

const Tape& TuringMachine::tape() const {
    return tape_;
}

std::size_t TuringMachine::stepCount() const {
    return stepCount_;
}

void TuringMachine::updateHaltedForCurrentState() {
    halted_ = isAcceptingState(acceptingStates_, currentState_);
}
