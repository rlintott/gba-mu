#include <string>
#include "GameBoyAdvance.h"
#include "ARM7TDMI.h"
#include "Bus.h"
#include "PPU.h"


void runCpuWithState() {
    PPU ppu;
    Bus bus{&ppu};
    ARM7TDMI cpu;
    GameBoyAdvance gba(&cpu, &bus);

    //cpu.setRegister(0, 0xEA00000C);
    cpu.setCurrInstruction(0xE25EF004);

    cpu.cpsr.T = 0;

    cpu.step();
}


int main() {
    runCpuWithState();
    return 0;
}

