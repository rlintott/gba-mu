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

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, Timer* _timer) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    arm7tdmi->connectBus(bus);
    this->timer = _timer;
    this->timer->connectBus(bus);
    this->timer->connectCpu(arm7tdmi);
}

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus) {
    DEBUG("initializing GBA\n");
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
    bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x1);
    bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x2);

    uint16_t currentScanline = -1;
    uint16_t nextScanline = 0;
    previousTime = getCurrentTime();
    startTimeSeconds = getCurrentTime() / 1000.0;

    // TODO: initialize this somewhere else
    bus->iORegisters[Bus::IORegister::KEYINPUT] = 0xFF;
    bus->iORegisters[Bus::IORegister::KEYINPUT + 1] = 0x03;

    while(true) {
        //DEBUGWARN(1 << "\n");
        uint32_t dmaCycles = dma->step(hBlank, vBlank, currentScanline);
        cyclesThisStep += dmaCycles;
        //DEBUGWARN(2 << "\n");
        timer->step(cyclesThisStep);
        //DEBUGWARN(3 << "\n");
        totalCycles += cyclesThisStep;
        cyclesThisStep = 0;

        vBlank = false;
        hBlank = false;
        cyclesThisStep += arm7tdmi->step();
        //DEBUGWARN(4 << "\n");
        if(totalCycles >= nextHBlank) { 
            hBlank = true;    

            if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x10) {
                    arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::HBlank);
            }
            // setting hblank flag to 1
            bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x2;

            nextHBlank += PPU::H_TOTAL;
        }
        //DEBUGWARN(5 << "\n");

        if(totalCycles >= nextHBlankEnd) {
            ppu->renderScanline(nextScanline);
            // setting hblank flag to 0
            currentScanline += 1;
            currentScanline %= 228;
            nextScanline = (currentScanline + 1) % 228;
            
            nextHBlankEnd += PPU::H_TOTAL;
            bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x2);
            if(currentScanline == ((uint16_t)(bus->iORegisters[Bus::IORegister::DISPSTAT + 1]))) {
                // current scanline == vcount bits in DISPSTAT
                // set vcounter flag
                bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x04;
                if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x20) {
                    // if vcount irq enabled, queue the interrupt!
                    //DEBUGWARN("VCOUNTER INTERRUPT at scanline " << currentScanline << "\n");
                    arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::VCounterMatch);
                }
            } else {
                // toggle vcounter flag off
                bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x04);

            }

            bus->iORegisters[Bus::IORegister::VCOUNT] = currentScanline;
        }
        //DEBUGWARN(6 << "\n");
        if(totalCycles >= nextVBlankEnd) {
            // setting vblank flag to 0
            nextVBlankEnd += PPU::V_TOTAL;
            bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x1);
        }

        if(totalCycles >= nextVBlank) {
            vBlank = true;
            // TODO: v blank interrupt if enabled
            if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x8) {
                arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::VBlank);
            }
            nextVBlank += PPU::V_TOTAL; 
            // TODO: clean this up
            // DEBUGWARN("frame!\n");
            // force a draw every frame
            bus->ppuMemDirty = true;
            screen->drawWindow(ppu->renderCurrentScreen());  
            Gamepad::getInput(bus);

            // setting vblank flag to 1
            bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x1;

            while(getCurrentTime() - previousTime < 17) {
                usleep(500);
            }
            previousTime = getCurrentTime();
            frames++;

            if((frames % 60) == 0) {
                DEBUGWARN("fps: " << (double)frames / ((getCurrentTime() / 1000.0) - startTimeSeconds) << "\n");
            }
        }
        //DEBUGWARN(7 << "\n");

    }
}

