#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"
#include "DMA.h"


int main(int argc, char *argv[]) {
    PPU ppu;
    Bus bus{&ppu};
    ARM7TDMI cpu;
    LCD screen;
    DMA dma;

    GameBoyAdvance gba(&cpu, &bus, &screen, &ppu, &dma);
    gba.loadRom(argv[1]);
    gba.loop();

    return 0;
}
