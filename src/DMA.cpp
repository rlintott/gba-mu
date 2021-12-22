#include "DMA.h"
#include "arm7tdmi/ARM7TDMI.h"
#include "memory/Bus.h"
#include "assert.h"
#include "GameBoyAdvanceImpl.h"
#include "PPU.h"
#include "Scheduler.h"
#include "assert.h"


// TODO: DMA specs not fully implemented yet
// TODO: fix mgba suite ROM Load DMA0 tests, which fail
uint32_t DMA::dmaX(uint8_t x, bool vBlank, bool hBlank, uint16_t scanline) {
    // TODO: optimization of this....
    // TODO: The 'Special' setting (Start Timing=3) depends on the DMA channel:DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
    uint32_t ioRegOffset =  0xC * x;

    // 40000BAh - DMA0CNT_H - DMA 0 Control (R/W)
    uint16_t control = (uint16_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_H + ioRegOffset]) |
                       (uint16_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1 + ioRegOffset] << 8);

    uint8_t startTiming = (control & 0x3000) >> 12;
    if(startTiming == 3) {
        if(x == 1 || x == 2) {
            // sound FIFO mode
            if(false /*TODO: if soundControllerRequest*/) {
            } else {
                return 0;
            }
        } else if(x == 3) {
            // video capture mode
            if(!hBlank) {
                DEBUGWARN("error! videoMode dma outside of hblank, this shouldn't be happening\n");
                return 0;
            }
            if((scanline != 2)) {
                // Capture works similar like HBlank DMA, however, the transfer is started when VCOUNT=2,
                // it is then repeated each scanline, and it gets stopped when VCOUNT=162
                scheduleDmaX(x, (control >> 8), false);
                return 0;
            }
        } else {
            assert(false);
        }
    }

    if(startTiming == 1) {
        if(!vBlank) {
            DEBUGWARN("error! vblank dma outside of vblank, this shouldn't be happening\n");
            return 0;
        }
    }
    if(startTiming == 2) {
        if(!hBlank) {
            DEBUGWARN("error! hblank dma outside of hblank, this shouldn't be happening\n");
        }
        if(scanline > PPU::SCREEN_HEIGHT - 1) {
            scheduleDmaX(x, (control >> 8), false);
            return 0;
        }   
    }

    // TODO: Game Pak DRQ  - DMA3 only -  (0=Normal, 1=DRQ <from> Game Pak, DMA3)    

    // TODO: implement this
    // if(startTiming == 3) {
    //     if(x == 3) {
    //         // video capture mode  using this transfer mode, set the repeat bit, 
    //         //and write the number of data units (per scanline) to the word count register. 
    //         // Capture works similar like HBlank DMA, however, the transfer is started when VCOUNT=2, 
    //         //it is then repeated each scanline, and it gets stopped when VCOUNT=162
    //         uint8_t vCount = bus->iORegisters[Bus::IORegister::VCOUNT];
    //         if((vCount != 2 && !inVideoCaptureMode) || vCount > 162) {
    //             inVideoCaptureMode = false;
    //             return 0;
    //         } else {
    //             inVideoCaptureMode = true;
    //         }
    //     } 
    // }

    bus->resetCycleCountTimeline();

    // TODO: TEMPORARY CYCLE COUNTING UNTIL WAITSTATES DONE PROPERLY
    uint32_t tempCycles = 2;

    // TODO: Internal time for DMA processing is 2I (normally), or 4I (if both source and destination are in gamepak memory area).
    //bus->addCycleToExecutionTimeline(Bus::INTERNAL, 0, 0);
    //bus->addCycleToExecutionTimeline(Bus::INTERNAL, 0, 0);

    if(!dmaXEnabled[x]) {
        dmaXEnabled[x] = true;

         // 40000B0h,0B2h - DMA0SAD - DMA 0 Source Address (W) (internal memory)
        dmaXSourceAddr[x] = (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + ioRegOffset]) |
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + 1 + ioRegOffset]) << 8) | 
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + 2 + ioRegOffset]) << 16) | 
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + 3 + ioRegOffset]) << 24);

        // 40000B4h,0B6h - DMA0DAD - DMA 0 Destination Address (W) (internal memory)
        dmaXDestAddr[x] = (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + ioRegOffset]) |
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 1 + ioRegOffset]) << 8) | 
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 2 + ioRegOffset]) << 16) | 
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 3 + ioRegOffset]) << 24);

        // 40000B8h - DMA0CNT_L - DMA 0 Word Count (W) (14 bit, 1..4000h)
        dmaXWordCount[x] = (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_L + ioRegOffset]) |
                            ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_L + 1 + ioRegOffset]) << 8);
        

        // SPECIAL BEHAVIOURS FOR DIFFERENT X
        switch(x) {
            case 0: {
                dmaXSourceAddr[x] &= internalMemMask;
                dmaXDestAddr[x] &= internalMemMask;

                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma012MaxWordCount;
                } else {
                    dmaXWordCount[x] &= dma012WordMask;
                }
                break;
            }
            case 1: {
                dmaXSourceAddr[x] &= anyMemMask;
                dmaXDestAddr[x] &= internalMemMask;

                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma012MaxWordCount;
                } else {
                    dmaXWordCount[x] &= dma012WordMask;
                }
                break;
            }
            case 2: {
                dmaXSourceAddr[x] &= anyMemMask;
                dmaXDestAddr[x] &= internalMemMask;

                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma012MaxWordCount;
                } else {
                    dmaXWordCount[x] &= dma012WordMask;
                }
                break;
            }
            case 3: {
                dmaXSourceAddr[x] &= anyMemMask;
                dmaXDestAddr[x] &= anyMemMask;

                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma3MaxWordCount;
                } else {
                    dmaXWordCount[x] &= dma3WordMask;
                }
                break;
            }
            default: {
                assert(false);
                break;
            }
        }

    }

    bool thirtyTwoBit = control & 0x0400; //  (0=16bit, 1=32bit)
    bool firstAccess = true;
 
    uint8_t destAdjust = (control & 0x0060) >> 5;
    uint8_t srcAdjust = (control & 0x0180) >> 7;
    assert(srcAdjust != 3);

    uint32_t offset = thirtyTwoBit ? 4 : 2;

    // writing / reading from memeory
    // TODO: implement DMA open bus (some games are dependent on it and wil help pass mgba test suite)

    for(uint32_t i = 0; i < dmaXWordCount[x]; i++) {
        if(thirtyTwoBit) {
            if(firstAccess) { 
                uint32_t value = bus->read32(dmaXSourceAddr[x] & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL);
                //uint32_t value = bus->read32(dmaXSourceAddr[x], Bus::NONSEQUENTIAL);
                bus->write32(dmaXDestAddr[x] & 0xFFFFFFFC, value, Bus::NONSEQUENTIAL);
                firstAccess = false;
            } else {
                uint32_t value = bus->read32(dmaXSourceAddr[x] & 0xFFFFFFFC, Bus::CycleType::SEQUENTIAL);
                bus->write32(dmaXDestAddr[x] & 0xFFFFFFFC, value, Bus::SEQUENTIAL);
            }
        } else {
            if(firstAccess) { 
                uint16_t value = bus->read16(dmaXSourceAddr[x] & 0xFFFFFFFE, Bus::CycleType::NONSEQUENTIAL);
                bus->write16(dmaXDestAddr[x] & 0xFFFFFFFE, value, Bus::NONSEQUENTIAL);
                firstAccess = false;
            } else {
                uint16_t value = bus->read16(dmaXSourceAddr[x] & 0xFFFFFFFE, Bus::CycleType::SEQUENTIAL);
                bus->write16(dmaXDestAddr[x] & 0xFFFFFFFE, value, Bus::SEQUENTIAL);
            }
        }

        // TODO: TEMPORARY CYCLE COUNTING UNTIL WAITSTATES DONE PROPERLY
        // iterating source memory pointer
        // (0=Increment,1=Decrement,2=Fixed,3=prohibited)
        switch(srcAdjust) {
            case 0: {
                dmaXSourceAddr[x] += offset;
                break;
            }
            case 1: {
                dmaXSourceAddr[x] -= offset;
                break;
            }
            case 2: {
                break;
            }
            default: {
                break;
            }
        }

        // iterating dest memory pointer
        // (0=Increment,1=Decrement,2=Fixed,3=Increment/Reload)
        switch(destAdjust) {
            case 0:
            case 3: {
                dmaXDestAddr[x] += offset;
                break;
            }
            case 1: {
                dmaXDestAddr[x] -= offset;
                break;
            }
            case 2: {
                break;
            }
            default: {
                break;
            }
        }
        GameBoyAdvanceImpl::cyclesSinceStart += 2;

        if(GameBoyAdvanceImpl::cyclesSinceStart >= scheduler->peekNextEvent()->startCycle) {
            // another event occurred during this dma! exit to handle that event
            // while scheduling this one immediately to resume after the event
            if((scheduler->peekNextEvent()->eventType) < Scheduler::convertDmaValToDmaEvent(x)) {
                scheduleDmaX(x, (control >> 8), true);
                dmaXWordCount[x] -= (i + 1);;
                return tempCycles;
            }
        }
    }
    tempCycles += bus->getMemoryAccessCycles();

    if(!(control & 0x0200)) {
        // DMA Repeat (0=Off, 1=On) (Must be zero if Bit 11 set)
        // if not dma repeat, set enable bit = 0 when done transfer
        bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1 + ioRegOffset] &= 0x7F;
        dmaXEnabled[x] = false;
    } else {
        // else, dma repeat is set, so schedule next dma
        if((startTiming == 2 && scanline >= (PPU::SCREEN_HEIGHT - 1)) || startTiming == 1 || 
           (startTiming == 3 && x == 3 && scanline >= 162))
            /* TODO: || (startTiming == 3 && (x == 1 || x == 2) && soundControllerFifoRequest) || 
                        (startTiming == 3 && x == 3 && scanline >= (PPU::SCREEN_HEIGHT - 1))*/ {
            // hblank repeat ends when in vblank, vlbank repeat only runs once per blank
            dmaXEnabled[x] = false;
        }
        dmaXWordCount[x] = (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_L + ioRegOffset]) |
                    ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_L + 1 + ioRegOffset]) << 8);
        if(destAdjust == 3) {
            dmaXDestAddr[x] = (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + ioRegOffset]) |
                    ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 1 + ioRegOffset]) << 8) | 
                    ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 2 + ioRegOffset]) << 16) | 
                    ((uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 3 + ioRegOffset]) << 24);
            // TODO: rules for masking dest addr 
        }

        uint16_t control = (uint16_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_H + ioRegOffset]) |
                    (uint16_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1 + ioRegOffset] << 8);
        
        scheduleDmaX(x, (control >> 8), false);

    }
    if(control & 0x4000) {
        //irq at end of word count
        switch(x) {
            case 0: {
                cpu->queueInterrupt(ARM7TDMI::Interrupt::DMA0);
                break;
            }
            case 1: {
                cpu->queueInterrupt(ARM7TDMI::Interrupt::DMA1);
                break;
            }
            case 2: {
                cpu->queueInterrupt(ARM7TDMI::Interrupt::DMA2);
                break;
            }
            case 3: {
                cpu->queueInterrupt(ARM7TDMI::Interrupt::DMA3);
                break;
            }
            default: {
                break;
            }
        }
    }

    return tempCycles;
}


void DMA::connectBus(std::shared_ptr<Bus> bus) {
    this->bus = bus;
}

void DMA::connectCpu(std::shared_ptr<ARM7TDMI> cpu) {
    this->cpu = cpu;
}


void DMA::updateDmaUponWrite(uint32_t address, uint32_t value, uint8_t width) {

    while(width != 0) {
        uint8_t byte = value & 0xFF;
        switch(address & 0xFF) {
            case 0xBB: {
                // dma 0
                scheduleDmaX(0, byte, false);
                break;
            }
            case 0xC7: {
                // dma 1
                scheduleDmaX(1, byte, false);
                break;
            }
            case 0xD3: {
                // dma 2
                scheduleDmaX(2, byte, false);
                break;
            }
            case 0xDF: {
                //dma 3
                scheduleDmaX(3, byte, false);
                break;
            }
            default: {
                //assert(false);
                break;
            }
        }
        width -= 8;
        address += 1;
        value = value >> 8;
    }
}

void DMA::scheduleDmaX(uint32_t x, uint8_t upperControlByte, bool immediately) {
    Scheduler::EventType eventType;
    switch(x) {
        case 0: {
            eventType = Scheduler::DMA0;
            break;
        }
        case 1: {
            eventType = Scheduler::DMA1;
            break;
        }
        case 2: {
            eventType = Scheduler::DMA2;
            break;
        }
        case 3: {
            eventType = Scheduler::DMA3;
            break;
        }
    }

    // remove old event
    scheduler->removeEvent(eventType);

    if((upperControlByte & 0x80) || immediately) {
        // enabling dma
        uint32_t ioRegOffset =  0xC * x;

        uint8_t startTiming = (upperControlByte & 0x30) >> 4;

        uint32_t cyclesInFuture = 2;
        if(immediately) {
            cyclesInFuture = 0;
        }

        switch(startTiming) {
            case 0: {
                scheduler->addEvent(eventType, cyclesInFuture, Scheduler::EventCondition::NULL_CONDITION, immediately);
                break;
            }
            case 1: {
                scheduler->addEvent(eventType, 0, Scheduler::EventCondition::VBLANK_START, immediately);
                break;
            }
            case 2: {
                scheduler->addEvent(eventType, 0, Scheduler::EventCondition::HBLANK_START, immediately);
                break;
            }
            case 3: {
                // special
                assert(x != 0);
                if(x == 1 || x == 2) {
                    scheduler->addEvent(eventType, cyclesInFuture, Scheduler::EventCondition::NULL_CONDITION, immediately);
                } else {
                    // x == 3
                    scheduler->addEvent(eventType, cyclesInFuture, Scheduler::EventCondition::DMA3_VIDEO_MODE, immediately);
                }

                break;
            }
        }

    } else {
        // disabling dma
        // don't need to add a new dma event
    }
}


void DMA::connectScheduler(std::shared_ptr<Scheduler> scheduler) {
    this->scheduler = scheduler;
}