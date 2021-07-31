#include "GameBoyAdvance.h"

#include <fstream>
#include <iostream>
#include <iterator>

#include "ARM7TDMI.h"
#include "Bus.h"
#include "LCD.h"
#include "PPU.h"

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    this->screen = _screen;
    arm7tdmi->connectBus(bus);
}

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    arm7tdmi->connectBus(bus);
}

GameBoyAdvance::~GameBoyAdvance() {}

void GameBoyAdvance::loadRom(std::string path) { 
    bus->loadRom(path); 
    arm7tdmi->initializeWithRom();
}

void GameBoyAdvance::testDisplay() {
    screen->initWindow();
}


void GameBoyAdvance::loop() {
    uint32_t totalCycles = 0;
    uint32_t cpuCycles = 0;
    uint32_t hScanProgress = 0;
    uint16_t currentScanline = 0;
    while(true) {
        cpuCycles = arm7tdmi->step();
        totalCycles += cpuCycles;
        hScanProgress = totalCycles % PPU::H_TOTAL;
        
        if(!hBlankInterruptCompleted && hScanProgress >= PPU::H_VISIBLE_CYCLES) {
            // TODO: h blank interrupt if enabled
            hBlankInterruptCompleted = true;
            bus->enterHBlank();
        }

        if(hScanProgress <= cpuCycles) {
            // just started a new scanline, so render it
            currentScanline++;
            currentScanline %= 228;
            ppu->renderScanline(currentScanline);
            hBlankInterruptCompleted = false;
        }

        if(!vBlankInterruptCompleted && currentScanline > 159) {
            // TODO: h blank interrupt if enabled
            vBlankInterruptCompleted = true;
            bus->enterVBlank();
        }


    }

}