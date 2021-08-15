#include "DMA.h"
#include "Bus.h"
#include "assert.h"




uint32_t DMA::dma(uint8_t x, bool vBlank, bool hBlank) {
    // 40000BAh - DMA0CNT_H - DMA 0 Control (R/W)
    uint16_t control = (uint32_t)bus->iORegisters[Bus::IORegister::DMA0CNT_H] |
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1] << 8);

    uint8_t startTiming = (control & 0x3000) >> 12;

    if((startTiming == 1 && !vBlank) || (startTiming == 2 && !hBlank)) {
        return 0;
    }

    bus->resetCycleCountTimeline();

    // TODO: TEMPORARY CYCLE COUNTING UNTIL WAITSTATES DONE PROPERLY
    uint32_t tempCycles = 2;

    // TODO: Internal time for DMA processing is 2I (normally), or 4I (if both source and destination are in gamepak memory area).
    bus->addCycleToExecutionTimeline(Bus::INTERNAL, 0, 0);
    bus->addCycleToExecutionTimeline(Bus::INTERNAL, 0, 0);

    // 40000B0h,0B2h - DMA0SAD - DMA 0 Source Address (W) (internal memory)
    // TODO: use helper fn
    uint32_t sourceAddr = (uint32_t)bus->iORegisters[Bus::IORegister::DMA0SAD] |
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + 1] << 8) | 
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + 2] << 18) | 
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0SAD + 3] << 24);

    // 40000B4h,0B6h - DMA0DAD - DMA 0 Destination Address (W) (internal memory)
    uint32_t destAddr = (uint32_t)bus->iORegisters[Bus::IORegister::DMA0DAD] |
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 1] << 8) | 
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 2] << 18) | 
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0DAD + 3] << 24);

    // 40000B8h - DMA0CNT_L - DMA 0 Word Count (W) (14 bit, 1..4000h)
    uint16_t wordCount = (uint32_t)bus->iORegisters[Bus::IORegister::DMA0CNT_L] |
                          (uint32_t)(bus->iORegisters[Bus::IORegister::DMA0CNT_L + 1] << 8);

    // SPECIAL BEHAVIOURS FOR DIFFERENT X
    switch(x) {
        case 0: {
            sourceAddr &= internalMemMask;
            destAddr &= internalMemMask;
            if(wordCount == 0) {
                wordCount = dma012MaxWordCount;
            }
            break;
        }
        case 1: {
            sourceAddr &= anyMemMask;
            destAddr &= internalMemMask;
            if(wordCount == 0) {
                wordCount = dma012MaxWordCount;
            }
            break;
        }
        case 2: {
            sourceAddr &= anyMemMask;
            destAddr &= internalMemMask;
            if(wordCount == 0) {
                wordCount = dma012MaxWordCount;
            }
            break;
        }
        case 3: {
            sourceAddr &= anyMemMask;
            destAddr &= anyMemMask;
            if(wordCount == 0) {
                wordCount = dma3MaxWordCount;
            }
            break;
        }
        default: {
            assert(false);
            break;
        }
    }

    bool thirtyTwoBit = control & 0x0400; //  (0=16bit, 1=32bit)
    bool firstAccess = true;
 
    //uint8_t destAdjust = control & 

    for(uint16_t i = 0; i < wordCount; i++) {
        if(thirtyTwoBit) {
            if(firstAccess) { 
                uint32_t value = bus->read32(sourceAddr, Bus::NONSEQUENTIAL);
                bus->write32(destAddr, value, Bus::NONSEQUENTIAL);
                firstAccess = false;
            } else {
                uint32_t value = bus->read32(sourceAddr, Bus::SEQUENTIAL);
                bus->write32(destAddr, value, Bus::SEQUENTIAL);
            }
            sourceAddr += 4;
            destAddr += 4;
            tempCycles++;

        } else {
            if(firstAccess) { 
                uint16_t value = bus->read16(sourceAddr, Bus::NONSEQUENTIAL);
                bus->write16(destAddr, value, Bus::NONSEQUENTIAL);
                firstAccess = false;
            } else {
                uint16_t value = bus->read16(sourceAddr, Bus::SEQUENTIAL);
                bus->write16(destAddr, value, Bus::SEQUENTIAL);
            }
            sourceAddr += 2;
            destAddr += 2;
            tempCycles++;
        }
    }

    if(!(control & 0x0200)) {
        // DMA Repeat (0=Off, 1=On) (Must be zero if Bit 11 set)
        // if not dma repeat, set enable bit = 0 when done transfer
        control &= 0x7FFF;
        bus->iORegisters[Bus::IORegister::DMA0CNT_L + 1] = (uint8_t)(control >> 8);
    }

    return tempCycles;
}


uint32_t DMA::step(bool hBlank, bool vBlank) {
    uint32_t cycles = 0;
    if(bus->iORegisters[Bus::IORegister::DMA0CNT_H + 1] & 0x80) {
        // dma0 enabled
    }
    if(bus->iORegisters[Bus::IORegister::DMA1CNT_H + 1] & 0x80) {
        // dma1 enabled
    }
    if(bus->iORegisters[Bus::IORegister::DMA2CNT_H + 1] & 0x80) {
        // dma2 enabled
    }
    if(bus->iORegisters[Bus::IORegister::DMA3CNT_H + 1] & 0x80) {
        // dma3 enabled
    }
}


