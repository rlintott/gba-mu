#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"
#include "DMA.h"


int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cerr << "Please include path to a GBA ROM" << std::endl;
    } else {
        PPU ppu;
        Bus bus{&ppu};
        ARM7TDMI cpu;
        LCD screen;
        DMA dma;

        GameBoyAdvance gba(&cpu, &bus, &screen, &ppu, &dma);
        if(gba.loadRom(argv[1])) {
            gba.loop();
        }
        else {
            return 1;
        }

        return 0;
    }
}
