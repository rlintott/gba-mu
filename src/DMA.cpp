#include "DMA.h"
#include "Bus.h"
#include "ARM7TDMI.h"
#include "assert.h"
#include "PPU.h"


// TODO: DMA specs not fully implemented yet
uint32_t DMA::dma(uint8_t x, bool vBlank, bool hBlank, uint16_t scanline) {
    // TODO: optimization of this....
    // TODO: The 'Special' setting (Start Timing=3) depends on the DMA channel:DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
    uint32_t ioRegOffset =  0xC * (uint32_t)x;

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
            if(!hBlank || (scanline != 2)) {
                // Capture works similar like HBlank DMA, however, the transfer is started when VCOUNT=2,
                // it is then repeated each scanline, and it gets stopped when VCOUNT=162
                return 0;
            }
        } else {
            assert(false);
        }
    }

    if(startTiming == 1) {
        if(!vBlank) {
            return 0;
        }
    }
    if(startTiming == 2) {
        if(!hBlank || scanline > PPU::SCREEN_HEIGHT - 1) {
            return 0;
        }
    }

    // TODO: Game Pak DRQ  - DMA3 only -  (0=Normal, 1=DRQ <from> Game Pak, DMA3)

    //DEBUGWARN("dma start\n");
    //DEBUGWARN(hBlank << "\n");
    //DEBUGWARN("x " << (uint32_t)x << "\n");
    //DEBUGWARN("control " << (uint32_t)control << "\n");
    //DEBUGWARN("startTiming: " << (uint32_t)startTiming << "\n");
    //DEBUGWARN("dma:  " << (uint32_t)x << "\n");
    

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
                //DEBUGWARN("src addr: " << dmaXSourceAddr[x] << "\n");
                //DEBUGWARN(ARM7TDMI::aluShiftRor(bus->read32(dmaXSourceAddr[x] & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL),
                //                                    (dmaXSourceAddr[x] & 3) * 8) << "\n");
                //exit(0);

                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma012MaxWordCount;
                }
                //DEBUGWARN("word count: " << dmaXWordCount[x] << "\n");
                break;
            }
            case 1: {
                dmaXSourceAddr[x] &= anyMemMask;
                dmaXDestAddr[x] &= internalMemMask;
                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma012MaxWordCount;
                }
                break;
            }
            case 2: {
                dmaXSourceAddr[x] &= anyMemMask;
                dmaXDestAddr[x] &= internalMemMask;
                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma012MaxWordCount;
                }
                break;
            }
            case 3: {
                dmaXSourceAddr[x] &= anyMemMask;
                dmaXDestAddr[x] &= anyMemMask;
                if(dmaXWordCount[x] == 0) {
                    dmaXWordCount[x] = dma3MaxWordCount;
                }
                break;
            }
            default: {
                assert(false);
                break;
            }
        }

    }

    // DEBUGWARN(dmaXWordCount[x] << " is wordCount \n"); 
    // DEBUGWARN((uint32_t)x << " is x \n"); 
    // if(x == 3 && dmaXDestAddr[x] >= 0xD000000) {
    //     //DEBUGWARN("gamepak rom writing\n");
    //     // TODO: temporarily disabling dma gamepak ROM writing. implement it
    //     //return 0;
    // }

    bool thirtyTwoBit = control & 0x0400; //  (0=16bit, 1=32bit)
    bool firstAccess = true;
 
    uint8_t destAdjust = (control & 0x0060) >> 5;
    uint8_t srcAdjust = (control & 0x0180) >> 7;
    assert(srcAdjust != 3);

    //DEBUGWARN(thirtyTwoBit << " is thirtyTwoBit \n"); 
    //DEBUGWARN(dmaXWordCount[x] << "\n");

    uint32_t offset = thirtyTwoBit ? 4 : 2;

    // writing / reading from memeory

    for(uint32_t i = 0; i < dmaXWordCount[x]; i++) {
        // DEBUGWARN(i << " is i \n"); 
        // DEBUGWARN(dmaXDestAddr[x] << " is dest \n"); 
        // DEBUGWARN(dmaXSourceAddr[x] << " is source \n"); 
        if(thirtyTwoBit) {
            if(firstAccess) { 
                uint32_t value = ARM7TDMI::aluShiftRor(bus->read32(dmaXSourceAddr[x] & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL),
                                                                   (dmaXSourceAddr[x] & 3) * 8);
                //uint32_t value = bus->read32(dmaXSourceAddr[x], Bus::NONSEQUENTIAL);
                //DEBUGWARN("value: " << value << "\n");
                bus->write32(dmaXDestAddr[x] & 0xFFFFFFFC, value, Bus::NONSEQUENTIAL);
                firstAccess = false;
            } else {
                uint32_t value = ARM7TDMI::aluShiftRor(bus->read32(dmaXSourceAddr[x] & 0xFFFFFFFC, Bus::CycleType::SEQUENTIAL),
                                                                   (dmaXSourceAddr[x] & 3) * 8);
                bus->write32(dmaXDestAddr[x] & 0xFFFFFFFC, value, Bus::SEQUENTIAL);
            }
        } else {
            if(firstAccess) { 
                uint16_t value = ARM7TDMI::aluShiftRor(bus->read16(dmaXSourceAddr[x] & 0xFFFFFFFE, Bus::CycleType::NONSEQUENTIAL),
                                                                   (dmaXSourceAddr[x] & 1) * 8);
                //DEBUGWARN("value: " << value << "\n");
                bus->write16(dmaXDestAddr[x] & 0xFFFFFFFE, value, Bus::NONSEQUENTIAL);
                firstAccess = false;
            } else {
                uint16_t value = ARM7TDMI::aluShiftRor(bus->read16(dmaXSourceAddr[x] & 0xFFFFFFFE, Bus::CycleType::SEQUENTIAL),
                                                                   (dmaXSourceAddr[x] & 1) * 8);
                //DEBUGWARN("value: " << value << "\n");
                bus->write16(dmaXDestAddr[x] & 0xFFFFFFFE, value, Bus::SEQUENTIAL);
            }
        }

        // TODO: TEMPORARY CYCLE COUNTING UNTIL WAITSTATES DONE PROPERLY

        // iterating source memory pointer
        // (0=Increment,1=Decrement,2=Fixed,3=prohibited)
        switch(srcAdjust) {
            case 0: {
                //DEBUGWARN("incrementing\n");
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
                //DEBUGWARN((uint32_t)destAdjust << " destAdjust\n");
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
        tempCycles += 2;
    }

    if(!(control & 0x0200)) {
        // DMA Repeat (0=Off, 1=On) (Must be zero if Bit 11 set)
        // if not dma repeat, set enable bit = 0 when done transfer
        //DEBUGWARN("no dma repeat: setting dma enabled to false\n");
        bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1 + ioRegOffset] &= 0x7F;
        dmaXEnabled[x] = false;
        //DEBUGWARN("imm after setting: " << (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_L + 1 + ioRegOffset]) << "\n");
    } else {
        // else, dma repeat is set, so 
        //DEBUGWARN("dma repeat: setting dma enabled to false\n");
        // 
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


    }
    if(control & 0x4000) {
        //irq at end of word count
        DEBUGWARN("irq\n");
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
    //DEBUGWARN("temp cycles " << tempCycles << "\n");
    //DEBUGWARN(bus->ppuMemDirty << "\n");

    return tempCycles;
}


uint32_t DMA::step(bool hBlank, bool vBlank, uint16_t scanline) {
    uint32_t cycles = 0;

    if(bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1] & 0x80) {
        // dma0 enabled
        cycles += dma(0, vBlank, hBlank, scanline);
    } 
    if(bus->iORegisters[Bus::IORegister::DMA1CNT_H + 1] & 0x80) {
        // dma1 enabled
        cycles += dma(1, vBlank, hBlank, scanline);
    } 
    if(bus->iORegisters[Bus::IORegister::DMA2CNT_H + 1] & 0x80) {
        // dma2 enabled
        cycles += dma(2, vBlank, hBlank, scanline);
    } 
    if(bus->iORegisters[Bus::IORegister::DMA3CNT_H + 1] & 0x80) {
        // dma3 enabled
        //DEBUGWARN((uint32_t)bus->iORegisters[Bus::IORegister::DMA3CNT_H + 1] << "\n");
        cycles += dma(3, vBlank, hBlank, scanline);
    } 
    return cycles;
}


void DMA::connectBus(Bus* bus) {
    this->bus = bus;
}

void DMA::connectCpu(ARM7TDMI* cpu) {
    this->cpu = cpu;
}
