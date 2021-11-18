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
#include "Timer.h"

using milliseconds = std::chrono::milliseconds;

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen, PPU* _ppu, DMA* _dma, Timer* _timer) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    this->screen = _screen;
    arm7tdmi->connectBus(bus);
    this->ppu = _ppu;
    ppu->connectBus(bus);
    this->dma = _dma;
    dma->connectBus(bus);
    dma->connectCpu(arm7tdmi);
    this->timer = _timer;
    this->timer->connectBus(bus);
    this->timer->connectCpu(arm7tdmi);
}

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    arm7tdmi->connectBus(bus);
}

GameBoyAdvance::~GameBoyAdvance() {}

// if rom loading successful return true, else return false
bool GameBoyAdvance::loadRom(std::string path) { 
    std::ifstream binFile(path, std::ios::binary);
    if(binFile.fail()) {
        std::cerr << "could not find file" << std::endl;
        return false;
    }
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    bus->loadRom(buffer); 
    arm7tdmi->initializeWithRom();
    return true;
}

void GameBoyAdvance::testDisplay() {
    screen->initWindow();
}

long getCurrentTime() {
    return  std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t GameBoyAdvance::getTotalCyclesElapsed() {
    return totalCycles;
}

void GameBoyAdvance::loop() {
    screen->initWindow();
    uint32_t cyclesThisStep = 0;
    uint64_t nextHBlank = PPU::H_VISIBLE_CYCLES;
    uint64_t nextVBlank = PPU::V_VISIBLE_CYCLES;
    uint64_t nextHBlankEnd = 0;
    uint64_t nextVBlankEnd = 227 * PPU::H_TOTAL;
    uint16_t currentScanline = 0;
    uint16_t nextScanline = 1;
    previousTime = getCurrentTime();
    startTimeSeconds = getCurrentTime() / 1000.0;

    // TODO: initialize this somewhere else
    bus->iORegisters[Bus::IORegister::KEYINPUT] = 0xFF;
    bus->iORegisters[Bus::IORegister::KEYINPUT + 1] = 0x03;

    while(true) {
        // TODO: move this PPU logic to a PPU method ie ppu->step(totalCycles, cpuCycles)
        uint32_t dmaCycles = dma->step(hBlank, vBlank, nextScanline);
        cyclesThisStep += dmaCycles;
        timer->step(cyclesThisStep);
        totalCycles += cyclesThisStep;
        cyclesThisStep = 0;

        vBlank = false;
        hBlank = false;
        if(!dmaCycles) {
            // dma did not occur
            //DEBUGWARN("cpu\n");
            //DEGUGWARN(cyclesThisStep << "\n");
            cyclesThisStep += arm7tdmi->step();
        }

        if(totalCycles >= nextHBlankEnd) {
            // setting hblank flag to 0
            nextHBlankEnd += PPU::H_TOTAL;
            bus->iORegisters[Bus::IORegister::DISPSTAT] &= 0xFD;
        }

        if(totalCycles >= nextHBlank) {
            // TODO: h blank interrupt if enabled
            if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x10) {
                arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::HBlank);
            }
    
            hBlank = true;
            // in case we have gone through multiple scanlines in a single step somehow
            uint16_t scanlinesThisStep = 1 + ((totalCycles - nextHBlank) / (uint64_t)PPU::H_TOTAL);
            currentScanline += scanlinesThisStep;
            currentScanline %= 228;
            nextScanline = (currentScanline + 1) % 228;
            
            bus->iORegisters[Bus::IORegister::VCOUNT] = currentScanline;
            // setting hblank flag to 1
            bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x2;

            nextHBlank += PPU::H_TOTAL;
            ppu->renderScanline(currentScanline); 
        }

        if(totalCycles >= nextVBlankEnd) {
            // setting vblank flag to 0
            nextVBlankEnd += PPU::V_TOTAL;
            bus->iORegisters[Bus::IORegister::DISPSTAT] &= 0xFE;
        }

        if(totalCycles >= nextVBlank) {
            // TODO: v blank interrupt if enabled
            if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x8) {
                arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::VBlank);
            }
            nextVBlank += PPU::V_TOTAL; 
            vBlank = true;
            // TODO: clean this up
            // DEBUGWARN("frame!\n");
            // force a draw every frame
            bus->ppuMemDirty = true;
            screen->drawWindow(ppu->renderCurrentScreen());  
            Gamepad::getInput(bus);

            // setting vblank flag to 1
            bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x1;

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

