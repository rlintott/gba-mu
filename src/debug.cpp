#include <string>
#include "ARM7TDMI.h"
#include "Bus.h"
#include "GameBoyAdvance.h"


void runCpuWithState() {
    Bus bus;
    ARM7TDMI cpu;
    GameBoyAdvance gba(&cpu, &bus);

    cpu.setCurrInstruction(0x0000DD2C);

    cpu.cpsr.T = 1;

    cpu.step();
}


int main() {
    runCpuWithState();
    return 0;
}

