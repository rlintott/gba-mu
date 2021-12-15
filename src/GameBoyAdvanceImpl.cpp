#include "GameBoyAdvanceImpl.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <iterator>
#include <chrono>
#include <algorithm> 
#include <future>
#include <thread>

#include "arm7tdmi/ARM7TDMI.h"
#include "memory/Bus.h"
#include "LCD.h"
#include "PPU.h"
#include "Gamepad.h"
#include "DMA.h"
#include "Timer.h"
#include "Debugger.h"
#include "Scheduler.h"

using milliseconds = std::chrono::milliseconds;

uint64_t GameBoyAdvanceImpl::cyclesSinceStart = 0;

GameBoyAdvanceImpl::GameBoyAdvanceImpl() {
    this->arm7tdmi = std::make_shared<ARM7TDMI>();
    this->bus =  std::make_shared<Bus>();
    this->screen =  std::make_shared<LCD>();
    arm7tdmi->connectBus(bus);
    this->ppu =  std::make_shared<PPU>();
    ppu->connectBus(bus);
    this->bus->connectPpu(ppu);
    this->dma =  std::make_shared<DMA>();
    bus->connectDma(dma);
    dma->connectBus(bus);
    dma->connectCpu(arm7tdmi);
    this->timer = std::make_shared<Timer>();
    this->timer->connectBus(bus);
    bus->connectTimer(timer);
    this->timer->connectCpu(arm7tdmi);
    this->scheduler =  std::make_shared<Scheduler>();
    dma->connectScheduler(scheduler);
    timer->connectScheduler(scheduler);
    this->debugger =  std::make_shared<Debugger>();
}

void GameBoyAdvanceImpl::printCpuState() {
    debugger->step(arm7tdmi.get(), bus.get());
    debugger->printState();
}

// if rom loading successful return true, else return false
bool GameBoyAdvanceImpl::loadRom(std::string path) { 
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

void GameBoyAdvanceImpl::testDisplay() {
    screen->initWindow();
}

long getCurrentTime() {
    return  std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t GameBoyAdvanceImpl::getTotalCyclesElapsed() {
    return totalCycles;
}

void GameBoyAdvanceImpl::enterMainLoop() {
    screen->initWindow();
    scheduler->addEvent(Scheduler::EventType::HBLANK, PPU::H_VISIBLE_CYCLES, Scheduler::EventCondition::NULL_CONDITION, false);
    scheduler->addEvent(Scheduler::EventType::VBLANK, PPU::V_VISIBLE_CYCLES, Scheduler::EventCondition::NULL_CONDITION, false);
    scheduler->addEvent(Scheduler::EventType::HBLANK_END, 0, Scheduler::EventCondition::NULL_CONDITION, false);
    scheduler->addEvent(Scheduler::EventType::VBLANK_END, 227 * PPU::H_TOTAL, Scheduler::EventCondition::NULL_CONDITION, false);
    scheduler->printEventList();
    bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x1);
    bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x2);

    uint16_t currentScanline = -1;
    uint16_t nextScanline = 0;
    uint64_t cyclesSinceLastScanline = 0;
    previousTime = getCurrentTime();
    double previous60Frame = getCurrentTime();
    startTimeSeconds = getCurrentTime() / 1000.0;

    // TODO: initialize this somewhere else
    bus->iORegisters[Bus::IORegister::KEYINPUT] = 0xFF;
    bus->iORegisters[Bus::IORegister::KEYINPUT + 1] = 0x03;
    double fps = 60.0;

    // STARTING MAIN EMULATION LOOP!
    while(true) {
        //uint32_t dmaCycles = dma->step(hBlank, vBlank, currentScanline);
        //cyclesThisStep += dmaCycles;
        if(debugMode) {
            debugger->step(arm7tdmi.get(), bus.get());
            if(debugger->stepMode) {
                while(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && debugMode && debugger->stepMode) {

                };
                while(!sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && debugMode && debugger->stepMode) {
                    if(sf::Keyboard::isKeyPressed(sf::Keyboard::X)) {
                        std::cout << "Leaving DEBUG mode!\n";
                        debugMode = false;
                        debugger->stepMode = false;
                        break;
                    }
                };
                debugger->printState();
            }
        }

       if(!bus->haltMode) {
            uint32_t cpuCycles = arm7tdmi->step();
            cyclesSinceStart += cpuCycles;
        } else {
            //DEBUGWARN("entering halt mode\n");
            if(((bus->iORegisters[Bus::IORegister::IE] & bus->iORegisters[Bus::IORegister::IF]) || 
               ((bus->iORegisters[Bus::IORegister::IE + 1] & 0x3F) & (bus->iORegisters[Bus::IORegister::IF + 1] & 0x3F)))) {
                // halt mode over, interrupt fired
                bus->haltMode = false;
            } else {
                // skip to next event
                //DEBUGWARN(scheduler->peekNextEventStartCycle() << " nextEventStartCycle\n");
                cyclesSinceStart = scheduler->peekNextEvent()->startCycle;
            }
        }

        Scheduler::Event* nextEvent = scheduler->getNextEvent(cyclesSinceStart, Scheduler::EventCondition::NULL_CONDITION);
        
        while(nextEvent != nullptr) {
            //DEBUGWARN(nextEvent << " next event\n");
            //DEBUGWARN(cyclesSinceStart << " cyclesSinceStart\n");
            //scheduler->printEventList();
            uint64_t eventCycles = 0;
            switch(nextEvent->eventType) {
                case Scheduler::EventType::DMA0: {
                    // TODO: put this repetive code into inline fn
                    if(nextEvent->eventCondition == Scheduler::EventCondition::NULL_CONDITION) {
                        eventCycles += dma->dmaX(0, false, false, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::HBLANK_START) {
                        eventCycles += dma->dmaX(0, false, true, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::VBLANK_START) {
                        eventCycles += dma->dmaX(0, true, false, currentScanline);
                    } else {
                        // EventConditoin = DMA3Videomode;
                        eventCycles += dma->dmaX(0, false, true, currentScanline);
                    }
                    break;
                }
                case Scheduler::EventType::DMA1: {

                    if(nextEvent->eventCondition == Scheduler::EventCondition::NULL_CONDITION) {
                        eventCycles += dma->dmaX(1, false, false, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::HBLANK_START) {
                        eventCycles += dma->dmaX(1, false, true, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::VBLANK_START) {
                        eventCycles += dma->dmaX(1, true, false, currentScanline);
                    } else {
                        // EventConditoin = DMA3Videomode;
                        eventCycles += dma->dmaX(1, false, true, currentScanline);
                    }                   
                    break;
                }
                case Scheduler::EventType::DMA2: {

                    if(nextEvent->eventCondition == Scheduler::EventCondition::NULL_CONDITION) {
                        eventCycles += dma->dmaX(2, false, false, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::HBLANK_START) {
                        eventCycles += dma->dmaX(2, false, true, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::VBLANK_START) {
                        eventCycles += dma->dmaX(2, true, false, currentScanline);
                    } else {
                        // EventCondition = DMA3Videomode;
                        eventCycles += dma->dmaX(2, false, true, currentScanline);
                    }                    
                    break;
                }
                case Scheduler::EventType::DMA3: {

                    if(nextEvent->eventCondition == Scheduler::EventCondition::NULL_CONDITION) {
                        eventCycles += dma->dmaX(3, false, false, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::HBLANK_START) {
                        eventCycles += dma->dmaX(3, false, true, currentScanline);
                    } else if(nextEvent->eventCondition == Scheduler::EventCondition::VBLANK_START) {
                        eventCycles += dma->dmaX(3, true, false, currentScanline);
                    } else {
                        // EventCondition = DMA3Videomode;
                        eventCycles += dma->dmaX(3, false, true, currentScanline);
                    }                  
                    break;
                }
                case Scheduler::EventType::TIMER0: {
                    timer->timerXOverflowEvent(0);
                    break;
                }
                case Scheduler::EventType::TIMER1: {
                    timer->timerXOverflowEvent(1);
                    break;
                }
                case Scheduler::EventType::TIMER2: {
                    timer->timerXOverflowEvent(2);
                    break;
                }
                case Scheduler::EventType::TIMER3: {
                    timer->timerXOverflowEvent(3);
                    break;
                }
                case Scheduler::EventType::VBLANK: {
                    //DEBUGWARN("vblank\n");
                    // vblank time!
                    // (do frame stuff)
                    // TODO: put some of this stuff to separate methods / classes
                    if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x8) {
                        arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::VBlank);
                    }
                    Gamepad::getInput(bus.get());

                    // setting vblank flag to 1
                    bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x1;

                    frames++;
                    
                    while(getCurrentTime() - previousTime < 17) {
                        usleep(500);
                    }

                    if((frames % 60) == 0) {
                        double smoothing = 0.8;
                        fps = fps * smoothing + ((double)60 / ((getCurrentTime() / 1000.0 - previous60Frame / 1000.0))) * (1.0 - smoothing);
                
                        std::cout << "fps: " << fps << "\n";
                        //DEBUGWARN("fps: " << ((double)frames / ((getCurrentTime() / 1000.0) - startTimeSeconds)) << "\n");
                        previous60Frame = previousTime;
                    }
                    previousTime = getCurrentTime();
                    screen->drawWindow(ppu->renderCurrentScreen());  

                    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
                        std::cout << "Entering DEBUG mode! Press LSHIFT to step through CPU instructions\n";
                        debugMode = true;
                        debugger->stepMode = true;
                    }
                    // add next vblank event
                    scheduler->addEvent(Scheduler::EventType::VBLANK, 
                                        PPU::V_TOTAL - ((cyclesSinceStart + PPU::V_VISIBLE_CYCLES) % PPU::V_TOTAL), 
                                        Scheduler::EventCondition::NULL_CONDITION, 
                                        false);                    
                    break;
                }
                case Scheduler::EventType::HBLANK: {      
                    // hblank time!
                    if(bus->iORegisters[Bus::IORegister::DISPSTAT] & 0x10) {
                        arm7tdmi->queueInterrupt(ARM7TDMI::Interrupt::HBlank);
                    }
                    bus->iORegisters[Bus::IORegister::DISPSTAT] |= 0x2;

                    // add next hblank event
                    scheduler->addEvent(Scheduler::EventType::HBLANK,
                                        PPU::H_TOTAL - ((cyclesSinceStart + PPU::H_VISIBLE_CYCLES) % PPU::H_TOTAL), 
                                        Scheduler::EventCondition::NULL_CONDITION,
                                        false);
                    
                    break;
                }
                case Scheduler::EventType::VBLANK_END: {
                    bus->iORegisters[Bus::IORegister::DISPSTAT] &= (~0x1);
                    // add next vblank end event
                    scheduler->addEvent(Scheduler::EventType::VBLANK_END, 
                                        PPU::V_TOTAL - (cyclesSinceStart % PPU::V_TOTAL), 
                                        Scheduler::EventCondition::NULL_CONDITION, 
                                        false);
                    break;
                }
                case Scheduler::EventType::HBLANK_END: {
                    ppu->renderScanline(nextScanline);
                    // setting hblank flag to 0
                    currentScanline += (cyclesSinceStart - cyclesSinceLastScanline) / PPU::H_TOTAL;    
                    cyclesSinceLastScanline = cyclesSinceStart - (cyclesSinceStart % PPU::H_TOTAL);
                    currentScanline %= 228;
                    nextScanline = (currentScanline + 1) % 228;
                    
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

                    // add next hblank end event
                    scheduler->addEvent(Scheduler::EventType::HBLANK_END, 
                                        PPU::H_TOTAL - (cyclesSinceStart % PPU::H_TOTAL), 
                                        Scheduler::EventCondition::NULL_CONDITION, 
                                        false);
                    break;
                }
                default: {
                    break;
                    //assert(false);
                }
            }
            //cyclesSinceStart += eventCycles;
            nextEvent = scheduler->getNextEvent(cyclesSinceStart, Scheduler::EventCondition::NULL_CONDITION);
        }
        
    }
}


ARM7TDMI* GameBoyAdvanceImpl::getCpu() {
    return arm7tdmi.get();
}




