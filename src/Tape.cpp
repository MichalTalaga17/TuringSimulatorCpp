#include "Tape.h"

#include <stdexcept>

std::string moveToString(HeadMove move) {
    switch (move) {
    case HeadMove::L:
        return "L";
    case HeadMove::R:
        return "P";
    case HeadMove::Stay:
        return "_";
    case HeadMove::STOP:
        return "STOP";
    default:
        throw std::runtime_error("Nieobslugiwany ruch glowicy");
    }
}

Tape::Tape(char blankSymbol) : blankSymbol_(blankSymbol) {}

void Tape::load(const std::string& input) {
    cells_.clear();
    headPosition_ = 0;

    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] != blankSymbol_) {
            cells_[static_cast<int>(i)] = input[i];
        }
    }
}

char Tape::read() const {
    const auto it = cells_.find(headPosition_);
    return it == cells_.end() ? blankSymbol_ : it->second;
}

void Tape::write(char symbol) {
    if (symbol == blankSymbol_) {
        cells_.erase(headPosition_);
        return;
    }

    cells_[headPosition_] = symbol;
}

void Tape::move(HeadMove moveDirection) {
    switch (moveDirection) {
    case HeadMove::L:
        --headPosition_;
        break;
    case HeadMove::R:
        ++headPosition_;
        break;
    case HeadMove::Stay:
    case HeadMove::STOP:
        break;
    default:
        throw std::runtime_error("Nieobslugiwany ruch glowicy");
    }
}

int Tape::headPosition() const {
    return headPosition_;
}

char Tape::blank() const {
    return blankSymbol_;
}

std::string Tape::snapshot(int from, int to) const {
    std::string result;
    result.reserve(static_cast<std::size_t>(to - from + 1));

    for (int position = from; position <= to; ++position) {
        const auto it = cells_.find(position);
        result.push_back(it == cells_.end() ? blankSymbol_ : it->second);
    }

    return result;
}
