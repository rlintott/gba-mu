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

using milliseconds = std::chrono::milliseconds;

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


long getCurrentTime() {
    return  std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void GameBoyAdvance::loop() {
    screen->initWindow();
    uint64_t totalCycles = 0;
    uint32_t cpuCycles = 0;
    uint32_t hScanProgress = 0;
    uint32_t vScanProgress = 0;
    uint16_t currentScanline = 0;
    previousTime = getCurrentTime();
    startTimeSeconds = getCurrentTime() / 1000.0;

    // TODO: initialize this somewhere else
    bus->iORegisters[Bus::IORegister::KEYINPUT] = 0xFF;
    bus->iORegisters[Bus::IORegister::KEYINPUT + 1] = 0x03;

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
            bus->iORegisters[Bus::IORegister::VCOUNT] = currentScanline;
            
            ppu->renderScanline(currentScanline);            
        }

        if(currentScanline >= PPU::SCREEN_HEIGHT - 1) {
            // TODO: h blank interrupt if enabled
        }

        if(vScanProgress - PPU::V_VISIBLE_CYCLES < cpuCycles && 
           vScanProgress - PPU::V_VISIBLE_CYCLES >= 0) {
            // TODO: clean this up
            // DEBUGWARN("frame!\n");
            // force a draw every frame
            ppu->setObjectsDirty();
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

