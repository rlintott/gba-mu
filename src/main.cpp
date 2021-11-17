#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"
#include "DMA.h"
#include "Timer.h"


int main(int argc, char *argv[]) {
    bool success = true;
    if(argc < 2) {
        std::cerr << "Please include path to a GBA ROM" << std::endl;
        success = false;
    } else {
        PPU ppu;
        Bus bus{&ppu};
        ARM7TDMI cpu;
        LCD screen;
        DMA dma;
        Timer timer;

        GameBoyAdvance gba(&cpu, &bus, &screen, &ppu, &dma, &timer);
        if(gba.loadRom(argv[1])) {
            gba.loop();
        }
        else {
            success = false;
        }
    }
    return success;
}
