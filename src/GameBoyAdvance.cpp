#include "GameBoyAdvance.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <iterator>
#include <chrono>
#include <algorithm> 

#include "ARM7TDMI.h"
#include "Bus.h"
#include "LCD.h"
#include "PPU.h"
#include "Gamepad.h"
#include "DMA.h"

using milliseconds = std::chrono::milliseconds;

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen, PPU* _ppu, DMA* _dma) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    this->screen = _screen;
    arm7tdmi->connectBus(bus);
    this->ppu = _ppu;
    ppu->connectBus(bus);
    this->dma = _dma;
    dma->connectBus(bus);
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


long getCurrentTime() {
    return  std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void GameBoyAdvance::loop() {
    screen->initWindow();
    uint64_t totalCycles = 0;
    uint32_t cyclesThisStep = 0;
    uint64_t nextHBlank = PPU::H_VISIBLE_CYCLES;
    uint64_t nextVBlank = PPU::V_VISIBLE_CYCLES;
    uint16_t currentScanline = 0;
    uint16_t nextScanline = 1;
    previousTime = getCurrentTime();
    startTimeSeconds = getCurrentTime() / 1000.0;

    // TODO: initialize this somewhere else
    bus->iORegisters[Bus::IORegister::KEYINPUT] = 0xFF;
    bus->iORegisters[Bus::IORegister::KEYINPUT + 1] = 0x03;

    while(true) {
        // TODO: move this PPU logic to a PPU method ie ppu->step(totalCycles, cpuCycles)
        cyclesThisStep = dma->step(hBlank, vBlank, nextScanline);
        vBlank = false;
        hBlank = false;
        if(!cyclesThisStep) {
            // dma did not occur
            //DEBUGWARN("cpu\n");
            //DEGUGWARN(cyclesThisStep << "\n");
            cyclesThisStep += arm7tdmi->step();
        }

        totalCycles += cyclesThisStep;

        if(totalCycles >= nextHBlank) {
            // TODO: h blank interrupt if enabled
            hBlank = true;
            // in case we have gone through multiple scanlines in a single step somehow
            uint16_t scanlinesThisStep = 1 + ((totalCycles - nextHBlank) / (uint64_t)PPU::H_TOTAL);
            currentScanline += scanlinesThisStep;
            currentScanline %= 228;
            nextScanline = (currentScanline + 1) % 228;
            
            bus->iORegisters[Bus::IORegister::VCOUNT] = currentScanline;
            nextHBlank += PPU::H_TOTAL;
            ppu->renderScanline(currentScanline); 
        }


        if(totalCycles >= nextVBlank) {
            // TODO: v blank interrupt if enabled
            nextVBlank += PPU::V_TOTAL; 
            vBlank = true;
            // TODO: clean this up
            // DEBUGWARN("frame!\n");
            // force a draw every frame
            bus->ppuMemDirty = true;
            screen->drawWindow(ppu->renderCurrentScreen());  
            Gamepad::getInput(bus);

            // #ifndef NDEBUGWARN
            // while(!sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
            //     screen->drawWindow(ppu->pixelBuffer);  
            // }
            // while(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter));
            // #endif

            while(getCurrentTime() - previousTime < 17) {
                usleep(500);
            }
            previousTime = getCurrentTime();
            frames++;

            if((frames % 60) == 0) {
                DEBUGWARN("fps: " << (double)frames / ((getCurrentTime() / 1000.0) - startTimeSeconds) << "\n");
            }
        }

    }
}

