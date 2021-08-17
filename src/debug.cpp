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

    cpu.setCurrInstruction(0xE14FB000);

    cpu.cpsr.T = 0;

    cpu.step();
}


int main() {
    runCpuWithState();
    return 0;
}

