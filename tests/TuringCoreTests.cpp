#include "Tape.h"
#include "TuringMachine.h"

#include <cassert>

namespace
{
void tapeReadsSparseCells()
{
    Tape tape('_');
    tape.load("10_");

    assert(tape.readAt(0) == '1');
    assert(tape.readAt(1) == '0');
    assert(tape.readAt(2) == '_');
    assert(tape.readAt(-1) == '_');
    assert(tape.snapshot(0, 2) == "10_");

    tape.move(HeadMove::R);
    tape.write('_');

    assert(tape.read() == '_');
    assert(tape.snapshot(0, 2) == "1__");
}

void loadTapeResetsExecutionState()
{
    TuringMachine machine('_');
    machine.setInitialState("q0");
    machine.addTransition("q0", '1', {'1', HeadMove::R, "q1"});
    machine.addTransition("q1", '_', {'_', HeadMove::STOP, "q1"});

    machine.loadTape("1");
    assert(machine.step());
    assert(machine.currentState() == "q1");
    assert(machine.step());
    assert(machine.isHalted());

    machine.loadTape("1");
    assert(!machine.isHalted());
    assert(machine.currentState() == "q0");
    assert(machine.stepCount() == 0);
    assert(machine.tape().headPosition() == 0);
}

void stopTransitionHaltsAfterOneExecutedStep()
{
    TuringMachine machine('_');
    machine.setInitialState("q0");
    machine.addTransition("q0", '_', {'_', HeadMove::STOP, "q0"});
    machine.loadTape("");

    assert(machine.step());
    assert(machine.isHalted());
    assert(machine.stepCount() == 1);
}
} // koniec przestrzeni nazw

int main()
{
    tapeReadsSparseCells();
    loadTapeResetsExecutionState();
    stopTransitionHaltsAfterOneExecutedStep();
    return 0;
}
