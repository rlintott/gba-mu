#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"


int main(int argc, char *argv[]) {
    PPU ppu;
    Bus bus{&ppu};
    ARM7TDMI cpu;
    LCD screen;

    GameBoyAdvance gba(&cpu, &bus, &screen, &ppu);
    gba.loadRom(argv[1]);
    gba.loop();

    return 0;
}
