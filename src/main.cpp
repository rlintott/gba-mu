#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"


int main() {
    Bus bus;
    ARM7TDMI cpu;
    LCD screen;
    PPU ppu;

    GameBoyAdvance gba(&cpu, &bus, &screen, &ppu);
    gba.loadRom("/Users/ryanlintott/Desktop/bin/key_demo.gba");
    gba.loop();

    std::cout << "completed!" << "\n";
    return 0;
}
