#include "GameBoyAdvance.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <iterator>

#include "ARM7TDMI.h"
#include "Bus.h"
#include "LCD.h"
#include "PPU.h"
#include "Gamepad.h"

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen, PPU* _ppu) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    this->screen = _screen;
    arm7tdmi->connectBus(bus);
    this->ppu = _ppu;
    ppu->connectBus(bus);
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
    screen->initWindow();
    uint64_t totalCycles = 0;
    uint32_t cpuCycles = 0;
    uint32_t hScanProgress = 0;
    uint32_t vScanProgress = 0;
    uint16_t currentScanline = 0;
    
    while(true) {
        cpuCycles = arm7tdmi->step();
        totalCycles += cpuCycles;
        hScanProgress = totalCycles % PPU::H_TOTAL;
        vScanProgress = totalCycles % PPU::V_TOTAL;

        // TODO: move this PPU logic to a PPU method ie ppu->step(totalCycles, cpuCycles)

        if(hScanProgress >= PPU::H_VISIBLE_CYCLES) {
            // TODO: h blank interrupt if enabled
        }

        if(hScanProgress < cpuCycles) {
            // just started a new scanline, so render it
            //DEBUGWARN(hScanProgress << "\n");
            //DEBUGWARN(totalCycles << "\n");
            currentScanline++;
            currentScanline = currentScanline % 228;
            ppu->renderScanline(currentScanline);
        }

        if(currentScanline >= PPU::SCREEN_HEIGHT - 1) {
            // TODO: h blank interrupt if enabled
        }

        if(vScanProgress < cpuCycles) {
            // finished all scanlines, render the whole screen
            //DEBUGWARN(vScanProgress << "\n");
            //DEBUGWARN(totalCycles << "\n");
            DEBUG("time to draw window!\n");
            screen->drawWindow(ppu->pixelBuffer);  
            Gamepad::getInput(bus);
        }
    }

}