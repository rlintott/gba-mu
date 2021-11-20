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

    // TODO: make initializing cpu states for debugging easier

    cpu.setRegister(0, 0x040000D2);
    cpu.setRegister(1, 0x0000B600);
    cpu.setRegister(2, 0x030056D0);
    cpu.setRegister(3, 0x00000000);
    cpu.setRegister(4, 0x0000B600);
    cpu.setRegister(5, 0x000000E0);
    cpu.setRegister(6, 0x00000000);
    cpu.setRegister(7, 0x00000000);
    cpu.setRegister(8, 0x03007EA0);
    cpu.setRegister(9, 0x03007EA2);
    cpu.setRegister(10, 0x00000000);
    cpu.setRegister(11, 0x00000000);
    cpu.setRegister(12, 0x000449A0);
    cpu.setRegister(13, 0x03007E54);
    cpu.setRegister(14, 0x080CEAB7);
    cpu.setRegister(14, 0x080CEC7C);
    cpu.setCurrInstruction(0x00008001);
    
    cpu.cpsr.T = 1;

    cpu.step();
}


int main() {
    runCpuWithState();
    return 0;
}

