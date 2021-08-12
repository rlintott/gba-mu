#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"


int main() {
    PPU ppu;
    Bus bus{&ppu};
    ARM7TDMI cpu;
    LCD screen;

    GameBoyAdvance gba(&cpu, &bus, &screen, &ppu);
    gba.loadRom("/Users/ryanlintott/Desktop/gba_dev/bin/obj_demo.gba");
    gba.loop();

    std::cout << "completed!" << "\n";
    return 0;

}
