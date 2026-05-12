#include "TuringMachine.h"

#include <iostream>

namespace {

// Wypisuje niewielki fragment taśmy wokół głowicy, żeby łatwo śledzić kolejne kroki.
void printMachine(const TuringMachine& machine) {
    const Tape& tape = machine.tape();
    const int headPosition = tape.headPosition();
    const int from = headPosition - 4;
    const int to = headPosition + 4;

    std::cout << "Krok: " << machine.stepCount() << '\n';
    std::cout << "Stan: " << machine.currentState() << '\n';
    std::cout << "Tasma: " << tape.snapshot(from, to) << '\n';
    std::cout << "       " << std::string(static_cast<std::size_t>(headPosition - from), ' ')
              << "^\n\n";
}

TuringMachine buildExampleMachine() {
    TuringMachine machine('_');
    machine.setInitialState("q0");
    machine.addAcceptingState("q1");

    machine.addTransition("q0", '1', {'0', HeadMove::R, "q0"});
    machine.addTransition("q0", '0', {'1', HeadMove::R, "q0"});
    machine.addTransition("q0", '_', {'S', HeadMove::STOP, "q1"});

    machine.loadTape("1001");
    return machine;
}

}

int main() {
    TuringMachine machine = buildExampleMachine();

    std::cout << "Przyklad maszyny Turinga\n";
    std::cout << "Dozwolone ruchy glowicy: "
              << moveToString(HeadMove::L) << ", "
              << moveToString(HeadMove::R) << ", "
              << moveToString(HeadMove::Stay) << ", "
              << moveToString(HeadMove::STOP) << "\n\n";
    printMachine(machine);

    while (machine.step()) {
        printMachine(machine);
    }

    std::cout << (machine.isAccepted() ? "Maszyna zaakceptowala slowo.\n"
                                       : "Maszyna zatrzymala sie.\n");
    return 0;
}
