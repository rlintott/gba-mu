#include "Bus.h"
#include "PPU.h"
#include "BIOS.h"
#include "Timer.h"
#include "DMA.h"
#include "ARM7TDMI.h"

#include "assert.h"

#include <fstream>
#include <iostream>
#include <iterator>

Bus::Bus(PPU* ppu) {
    // TODO: make bios configurable
    DEBUG("initializing bus\n");


    for(int i = 0; i < 98688; i++) {
        vRam.push_back(0);
    }
    for(int i = 0; i < BIOS::size; i++) {
        bios.push_back(BIOS::data[i]);
    }
    for(int i = 0; i < 263168; i++) {
        wRamBoard.push_back(0);
    }
    for(int i = 0; i < 32896; i++) {
        wRamChip.push_back(0);
    }
    for(int i = 0; i < 1028; i++) {
        iORegisters.push_back(0);
    }
    // Initialized to 0D000020h (by hardware). Unlike all other I/O registers, 
    // this register is mirrored across the whole I/O area (in increments of 64K, 
    // ie. at 4000800h, 4010800h, 4020800h, ..., 4FF0800h)
    // TODO:
    // iORegisters[INTERNAL_MEM_CNT] = 0x20;
    // iORegisters[INTERNAL_MEM_CNT + 1] = 0x00;
    // iORegisters[INTERNAL_MEM_CNT + 2] = 0x00;
    // iORegisters[INTERNAL_MEM_CNT + 3] = 0x0D;

    for(int i = 0; i < 1028; i++) {
        paletteRam.push_back(0);
    }
    for(int i = 0; i < 1028; i++) {
        objAttributes.push_back(0);
    }
    for(int i = 0; i < 69000; i++) {
        gamePakSram.push_back(0);
    }
    // TODO, use resize fn for initialization
    gamePakRom.resize(32000000);
    this->ppu = ppu;
}

Bus::~Bus() {
}

inline
uint32_t readFromArray32(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift) {
        return (uint32_t)arr->at(address - shift) |
               (uint32_t)arr->at((address + 1) - shift) << 8 |
               (uint32_t)arr->at((address + 2) - shift) << 16 |
               (uint32_t)arr->at((address + 3) - shift) << 24;
}

inline
uint16_t readFromArray16(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift) {
        return (uint16_t)arr->at(address - shift) |
               (uint16_t)arr->at((address + 1) - shift) << 8;
}

inline
uint8_t readFromArray8(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift) {
        return (uint8_t)arr->at(address - shift);
}

inline
void writeToArray32(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift, uint32_t value) {
        arr->at(address - shift) = (uint8_t)value;
        arr->at((address + 1) - shift) = (uint8_t)(value >> 8);
        arr->at((address + 2) - shift) = (uint8_t)(value >> 16);
        arr->at((address + 3) - shift) = (uint8_t)(value >> 24);
}

inline
void writeToArray16(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift, uint16_t value) {
        arr->at(address - shift) = (uint8_t)value;
        arr->at((address + 1) - shift) = (uint8_t)(value >> 8);
}

inline
void writeToArray8(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift, uint8_t value) {
        arr->at(address - shift) = (uint8_t)value;
}

void Bus::addCycleToExecutionTimeline(CycleType cycleType, uint32_t shift, uint8_t width) {
    if(cycleType == CycleType::INTERNAL) {
        executionTimelineCycles[executionTimelineSize] = 1;
        executionTimelineCycleType[executionTimelineSize] = cycleType;
        executionTimelineSize++; 
        return;                        
    }

    switch(shift) {
        case 0: // bios
        case 0x03000000:  // CHIP RAM    
        case 0x04000000: { // IO
            executionTimelineCycles[executionTimelineSize] = 1;
            executionTimelineCycleType[executionTimelineSize] = cycleType;
            executionTimelineSize++;            
            break;
        }       
        case 0x02000000: { // BOARD RAM
            switch(width) {
                case 32: {
                    executionTimelineCycles[executionTimelineSize] = 6;
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
                case 16:
                case 8: {
                    executionTimelineCycles[executionTimelineSize] = 3;
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
            }
            break;
        }       
        case 0x05000000:  // palette RAM       
        case 0x06000000: { // VRAM
            switch(width) {
                case 32: {
                    executionTimelineCycles[executionTimelineSize] = 2;
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
                case 16:
                case 8: {
                    executionTimelineCycles[executionTimelineSize] = 1;
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
            }
            break;

        }       
        case 0x07000000: { // OBJ Attributes
            executionTimelineCycles[executionTimelineSize] = 1;
            executionTimelineCycleType[executionTimelineSize] = cycleType;
            executionTimelineSize++;            
            break;
        }       
        case 0x08000000: { // gamepak waitstate 0
            switch(width) {
                case 32: {
                    if(cycleType == SEQUENTIAL) {
                        executionTimelineCycles[executionTimelineSize] = getWaitState0SCycles() + 1 + 
                                                                        getWaitState0SCycles() + 1;
                    } else {
                        executionTimelineCycles[executionTimelineSize] = getWaitState0NCycles() + 1 + 
                                                                        getWaitState0SCycles() + 1;
                    }
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
                case 16:
                case 8: {
                    if(cycleType == SEQUENTIAL) {
                        executionTimelineCycles[executionTimelineSize] = getWaitState0SCycles() + 1;
                    } else {
                        executionTimelineCycles[executionTimelineSize] = getWaitState0NCycles() + 1;
                    }
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
            }  
            break; 
        } 
        case 0x0A000000: { // gamepak waitstate 1
            switch(width) {
                case 32: {
                    if(cycleType == SEQUENTIAL) {
                        executionTimelineCycles[executionTimelineSize] = getWaitState1SCycles() + 1 + 
                                                                        getWaitState1SCycles() + 1;
                    } else {
                        executionTimelineCycles[executionTimelineSize] = getWaitState1NCycles() + 1 + 
                                                                        getWaitState1SCycles() + 1;
                    }
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
                case 16:
                case 8: {
                    if(cycleType == SEQUENTIAL) {
                        executionTimelineCycles[executionTimelineSize] = getWaitState1SCycles() + 1;
                    } else {
                        executionTimelineCycles[executionTimelineSize] = getWaitState1NCycles() + 1;
                    }
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
            }   
            break;
        }  
        case 0x0C000000: { // gamepak waitstate 2
            switch(width) {
                case 32: {
                    if(cycleType == SEQUENTIAL) {
                        executionTimelineCycles[executionTimelineSize] = getWaitState2SCycles() + 1 + 
                                                                        getWaitState2SCycles() + 1;
                    } else {
                        executionTimelineCycles[executionTimelineSize] = getWaitState2NCycles() + 1 + 
                                                                        getWaitState2SCycles() + 1;
                    }
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
                case 16:
                case 8: {
                    if(cycleType == SEQUENTIAL) {
                        executionTimelineCycles[executionTimelineSize] = getWaitState2SCycles() + 1;
                    } else {
                        executionTimelineCycles[executionTimelineSize] = getWaitState2NCycles() + 1;
                    }
                    executionTimelineCycleType[executionTimelineSize] = cycleType;
                    executionTimelineSize++;
                    break;
                }
            }   
            break;        
        } 
        case 0x0E000000: {
            if(width == 8) {
                executionTimelineCycles[executionTimelineSize] = 5;
                executionTimelineCycleType[executionTimelineSize] = cycleType;
                executionTimelineSize++;                  
            }
            break;
        } 
    }
}

inline
uint32_t Bus::read(uint32_t address, uint8_t width, CycleType cycleType) {
    // TODO: use same switch statement pattern as in fn addCycleToExecutionTimeline
    memAccessCycles += 1;
    uint32_t shift = (address & 0xFF000000) >> 24;
    //addCycleToExecutionTimeline(cycleType, address & 0x0F000000, width);

    switch(shift) {
        case 0x0:
        case 0x01: {
            if(0x00004000 <= address && address <= 0x01FFFFFF)  {
                //DEBUGWARN("reading from unused memory! address " << address << "\n");
                break;
            }
            switch(width) {
                case 32: {
                    return readFromArray32(&bios, address, 0);
                }
                case 16: {
                    return readFromArray16(&bios, address, 0);
                }
                case 8: {
                    return readFromArray8(&bios, address, 0);
                }
                default: {
                    assert(false);
                    break;
                }
                break;
            }
            break;
        }
        case 0x02: { // board RAM 
            DEBUG("reading from wramBoard\n");
            address &= 0x0203FFFF;
            switch(width) {
                case 32: {
                    memAccessCycles += 5;
                    return readFromArray32(&wRamBoard, address, 0x02000000);
                }
                case 16: {
                    memAccessCycles += 2;
                    return readFromArray16(&wRamBoard, address, 0x02000000);            
                }
                case 8: {
                    memAccessCycles += 2;
                    return readFromArray8(&wRamBoard, address, 0x02000000);            
                }
                default: {
                    assert(false);
                    break;
                }
            }
            break;
        }   
        case 0x03: {   
            // mirrored every 8000 bytes
            address &= 0x03007FFF;
            //DEBUGWARN("start\n");
            DEBUG("reading from wramChip addr: " << address << "\n");
            if(address > 0x03007FFF) {
                break;
            }
            switch(width) {
                case 32: {
                    return readFromArray32(&wRamChip, address, 0x03000000);
                }
                case 16: {
                    return readFromArray16(&wRamChip, address, 0x03000000);            
                }
                case 8: {
                    return readFromArray8(&wRamChip, address, 0x03000000);            
                }
                default: {
                    assert(false);
                    break;
                }
            }     
            //DEBUGWARN("end\n");
            break;
        }      
        case 0x04: {
            if(address > 0x040003FE) {
                break;
            }
            if(0x4000100 <= address && address <= 0x400010E) {
                // timer addresses
                timer->updateBusToPrepareForTimerRead(address, width);
            }

            switch(width) {
                case 32: {
                    return readFromArray32(&iORegisters, address, 0x04000000);
                }
                case 16: {
                    return readFromArray16(&iORegisters, address, 0x04000000);            }
                case 8: {
                    return readFromArray8(&iORegisters, address, 0x04000000);            }
                default: {
                    assert(false);
                    break;
                }
            }    
            break;       
        }     
        case 0x05: {  
            // if(address > 0x050003FF) {
            //     break;
            // }  
            address &= 0x050003FF;
            switch(width) {
                case 32: {
                    return readFromArray32(&paletteRam, address, 0x05000000);
                }
                case 16: {
                    return readFromArray16(&paletteRam, address, 0x05000000);            
                }
                case 8: {
                    return readFromArray8(&paletteRam, address, 0x05000000);            
                }
                default: {
                    assert(false);
                    break;
                }
            } 
            break;
        }    
        case 0x06: {    
            // Even though VRAM is sized 96K (64K+32K), it is repeated in steps of 128K 
            // (64K+32K+32K, the two 32K blocks itself being mirrors of each other).
            //address &= 0x06017FFF;
            if(address & 0x00010000) {
                address &= 0x06007FFF;
                address += 0x10000;
            } else {
                address &= 0x0600FFFF;
            }
            switch(width) {
                case 32: {
                    return readFromArray32(&vRam, address, 0x06000000);
                }
                case 16: {
                    return readFromArray16(&vRam, address, 0x06000000);            
                }
                case 8: {
                    return readFromArray8(&vRam, address, 0x06000000);            
                }
                default: {
                    assert(false);
                    break;
                }
            }    
            break;
        }        
        case 0x07: {   
            address &= 0x070003FF;
            switch(width) {
                case 32: {
                    return readFromArray32(&objAttributes, address, 0x07000000);
                }
                case 16: {
                    return readFromArray16(&objAttributes, address, 0x07000000);            
                }
                case 8: {
                    return readFromArray8(&objAttributes, address, 0x07000000);           
                }
                default: {
                    assert(false);
                    break;
                }
            }
            break;
        }      
        case 0x08:
        case 0x09: {
            //  TODO: *** Separate timings for sequential, and non-sequential accesses.
            // waitstate 0 
            // if(address - 0x08000000 > gamePakRom.size()) {
            //     DEBUGWARN("bigger!\n");
            //     DEBUGWARN(address - 0x08000000 << "\n");
            //     DEBUGWARN(gamePakRom.size() << " <- gamepakrom size\n");
            // }
            DEBUG("reading from gamepak\n");
            switch(width) {
                case 32: {
                    memAccessCycles += 7;
                    return readFromArray32(&gamePakRom, address, 0x08000000);
                }
                case 16: {
                    memAccessCycles += 4;
                    return readFromArray16(&gamePakRom, address, 0x08000000);            
                }
                case 8: {  
                    memAccessCycles += 4; 
                    return readFromArray8(&gamePakRom, address, 0x08000000);           
                }
                default: {
                    assert(false);
                    break;
                }
            } 
            DEBUG("done reading from gamepak\n");
            break;  
        } 
        case 0x0A:
        case 0x0B: {
            //  TODO: *** Separate timings for sequential, and non-sequential accesses.
            // waitstate 1
            switch(width) {
                case 32: {
                    return readFromArray32(&gamePakRom, address, 0x0A000000);
                }
                case 16: {
                    return readFromArray16(&gamePakRom, address, 0x0A000000);            
                }
                case 8: {
                    return readFromArray8(&gamePakRom, address, 0x0A000000);            
                }
                default: {
                    assert(false);
                    break;
                }
            }   
            break;
        } 
        case 0x0C:
        case 0x0D:  {
            //  TODO: *** Separate timings for sequential, and non-sequential accesses.
            // waitstate 2
            switch(width) {
                case 32: {
                    return readFromArray32(&gamePakRom, address, 0x0C000000);
                }
                case 16: {
                    return readFromArray16(&gamePakRom, address, 0x0C000000);           
                }
                case 8: {
                    return readFromArray8(&gamePakRom, address, 0x0C000000);            
                }
                default: {
                    assert(false);
                    break;
                }
            } 
            break;  
        }
        case 0x0E:
        case 0x0F:  {
            // The 64K SRAM area is mirrored across the whole 32MB area at E000000h-FFFFFFFh, 
            // also, inside of the 64K SRAM field, 32K SRAM chips are repeated twice.
            address &= 0x0E00FFFF;

            switch(width) {
                case 32: {
                    return ((uint32_t)readFromArray8(&gamePakSram, address, 0x0E000000)) * 0x01010101;      
                }
                case 16: {
                    return ((uint16_t)readFromArray8(&gamePakSram, address, 0x0E000000)) * (uint16_t)0x0101;    
                }
                case 8: {
                    return readFromArray8(&gamePakSram, address, 0x0E000000);           
                }
                default: {
                    assert(false);
                    break;
                }
            } 

            break;
        }
        default: {  
            // TODO: implement unused memory access behaviour (and check for unused memory writes)
            if(0x10000000 <= address && address <= 0xFFFFFFFF) {
                //DEBUGWARN("reading from unused memory! address " << address << "\n");
            }
            break;
        }

    }

    return 436207618U;
}

inline
void Bus::write(uint32_t address, uint32_t value, uint8_t width, CycleType accessType) {
    // TODO: use same switch statement pattern as in fn addCycleToExecutionTimeline
    //addCycleToExecutionTimeline(accessType, address & 0x0F000000, width);
    memAccessCycles += 1;
    uint32_t shift = (address & 0xFF000000) >> 24;

    switch(shift) {
        case 0x0:
        case 0x01: {    
            break;   
        }
        case 0x02: { // BOARD RAM  
            address &= 0x0203FFFF;
            //DEBUGWARN("start write at 0x02000000\n");
            switch(width) {
                case 32: {
                    memAccessCycles += 5;
                    writeToArray32(&wRamBoard, address, 0x02000000, value);
                    break;
                }
                case 16: {
                    memAccessCycles += 2;
                    writeToArray16(&wRamBoard, address, 0x02000000, value);    
                    break;        
                }
                case 8: {
                    memAccessCycles += 2;
                    writeToArray8(&wRamBoard, address, 0x02000000, value);    
                    break;       
                }
                default: {
                    assert(false);
                    break;
                }
            }
            break; 
        }
        case 0x03: {
            // mirrored every 8000 bytes
            address &= 0x03007FFF;
            switch(width) {
                case 32: {
                    writeToArray32(&wRamChip, address, 0x03000000, value); 
                    break;
                }
                case 16: {
                    writeToArray16(&wRamChip, address, 0x03000000, value);       
                    break;     
                }
                case 8: {
                    writeToArray8(&wRamChip, address, 0x03000000, value);             
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            } 
            //DEBUGWARN("end\n");    
            //DEBUGWARN("done writing to ram chip\n");
            break;        
        }
        case 0x04: {
            if(0x04000000 <= address && address < 0x04000056) {
            }

            if(0x4000100 <= address && address <= 0x400010F) {
                // timer addresses
                //DEBUGWARN("howdy\n");
                timer->updateTimerUponWrite(address, value, width);
            }

            // TODO: there's a more efficient way to do this I think,
            // send the changed register to DMA AFTER the write happens
            if(0x40000BA <= address && address <= 0x40000DF) {
                // dma addresses
                dma->updateDmaUponWrite(address, value, width);
            }

            // DEBUGWARN("width " << (uint32_t)width << "\n");
            // DEBUGWARN("value " << value << "\n");
            // DEBUGWARN("addr " << address << "\n");

            switch(width) {
                case 32: {
                    writeToArray32(&iORegisters, address, 0x04000000, value); 
                    break;
                }
                case 16: {
                    writeToArray16(&iORegisters, address, 0x04000000, value);      
                    break;      
                }
                case 8: {
                    writeToArray8(&iORegisters, address, 0x04000000, value);         
                    break;  
                }
                default: {
                    assert(false);
                    break;
                }
            } 

            // SPECIAL CASE when writing to interrupt request register
            // setting a bit (acknowledging an interrupt) changes that bit to zero
            // so do val &= ~val
            if(0x4000200 <= address && address <= 0x4000203) {
                //DEBUGWARN("eyo1\n");
                uint8_t tempWidth = width;
                uint32_t tempAddress = address;
                uint32_t tempValue = value;
                while(tempWidth != 0) {
                    //DEBUGWARN("heyo\n");
                    //DEBUGWARN("tempAddress " << tempAddress << "\n");
                    
                    if(tempAddress == 0x04000202 || tempAddress == 0x04000203) {
                        //DEBUGWARN("before " << (uint32_t)iORegisters[tempAddress - 0x04000000] << "\n");
                        iORegisters[tempAddress - 0x04000000] = iORegisters[tempAddress - 0x04000000] & (~tempValue);
                        //DEBUGWARN("after " << (uint32_t)iORegisters[tempAddress - 0x04000000] << "\n");
                    }
                    
                    tempWidth -= 8;
                    tempAddress += 1;
                    tempValue = tempValue >> 8;
                }
                //DEBUGWARN("eyo2\n");
            }   

            if(address == 0x04000301) {
                // halt register hit
                //DEBUGWARN("hey!\n");
                if(!(iORegisters[HALTCNT] & 0x80)) {
                    haltMode = true;
                }
            }           
            break;
        }
        case 0x05: {  
            address &= 0x050003FF;
            switch(width) {
                case 32: {
                    writeToArray32(&paletteRam, address, 0x05000000, value); 
                    break;
                }
                case 16: {
                    writeToArray16(&paletteRam, address, 0x05000000, value);         
                    break;    
                }
                case 8: {
                    /*
                    Writes to BG (6000000h-600FFFFh) (or 6000000h-6013FFFh in Bitmap mode) 
                    and to Palette (5000000h-50003FFh) are writing the new 8bit value to
                    BOTH upper and lower 8bits of the addressed halfword, ie. "[addr AND NOT 1]=data*101h".
                    */
                    writeToArray16(&paletteRam, address & 0xFFFFFFFE, 0x05000000, value * 0x101); 
                    break;            
                }
                default: {
                    assert(false);
                    break;
                }
            } 
            break;
        } 
        case 0x06: {  
            // DEBUGWARN("vram: writing " << value << " to " << address - 0x06000000 << "\n");
            // Even though VRAM is sized 96K (64K+32K), it is repeated in steps of 128K 
            // (64K+32K+32K, the two 32K blocks itself being mirrors of each other).
                if(address & 0x00010000) {
                    address &= 0x06007FFF;
                    address += 0x10000;
                } else {
                    address &= 0x0600FFFF;
                }
            //address = (address & 0x0607FFF);
            switch(width) {
                case 32: {
                    writeToArray32(&vRam, address, 0x06000000, value);
                    break;
                }
                case 16: {
                    writeToArray16(&vRam, address, 0x06000000, value);    
                    break;        
                }
                case 8: {
                    writeToArray8(&vRam, address, 0x06000000, value);      
                    break;      
                }
                default: {
                    assert(false);
                    break;
                }
            }
        
            break;
        } 
        case 0x07: {   
            // TODO: there are more hblank rules to implement
            address &= 0x070003FF;
            switch(width) {
                case 32: {
                    writeToArray32(&objAttributes, address, 0x07000000, value);
                    break;
                }
                case 16: {
                    writeToArray16(&objAttributes, address, 0x07000000, value);          
                    break;  
                }
                case 8: {
                    // 8 bit Writes to OBJ (6010000h-6017FFFh) (or 6014000h-6017FFFh in Bitmap mode) 
                    // and to OAM (7000000h-70003FFh) are ignored, the memory content remains unchanged.
                    // writeToArray8(&objAttributes, address, 0x07000000, value);            
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }
            break;   

        }
        case 0x08:
        case 0x09: {
            //  TODO: *** Separate timings for sequential, and non-sequential accesses.
            // waitstate 0

            if(address - 0x08000000 > gamePakRom.size()) {
                DEBUGWARN("bigger!\n");
                DEBUGWARN(address - 0x08000000 << "\n");
            }
            //DEBUGWARN("writing to gampak\n");

            switch(width) {
                case 32: {
                    //writeToArray32(&gamePakRom, address, 0x08000000, value);
                    break;
                }
                case 16: {
                    //writeToArray16(&gamePakRom, address, 0x08000000, value);         
                    break;   
                }
                case 8: {
                    //writeToArray8(&gamePakRom, address, 0x08000000, value);          
                    break;  
                }
                default: {
                    assert(false);
                    break;
                }
            }   

            break;
        }
        case 0x0A:
        case 0x0B: { 
            //  TODO: *** Separate timings for sequential, and non-sequential accesses.
            // waitstate 1
            //assert(false);
            switch(width) {
                case 32: {
                    //writeToArray32(&gamePakRom, address, 0x0A000000, value);
                    break;
                }
                case 16: {
                    //writeToArray16(&gamePakRom, address, 0x0A000000, value);         
                    break;   
                }
                case 8: {
                    //writeToArray8(&gamePakRom, address, 0x0A000000, value);           
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }   
            break;
        }
        case 0x0C:
        case 0x0D:  {
            //  TODO: *** Separate timings for sequential, and non-sequential accesses.
            // waitstate 2
            //assert(false);
            switch(width) {
                case 32: {
                    //writeToArray32(&gamePakRom, address, 0x0C000000, value);
                    break;
                }
                case 16: {
                    //writeToArray16(&gamePakRom, address, 0x0C000000, value);            
                    break;
                }
                case 8: {
                    //writeToArray8(&gamePakRom, address, 0x0C000000, value);            
                    break;
                }
                default: {
                    assert(false);
                }
            }   
            break;
        }
        case 0x0E:
        case 0x0F:  {
            // The 64K SRAM area is mirrored across the whole 32MB area at E000000h-FFFFFFFh, 
            // also, inside of the 64K SRAM field, 32K SRAM chips are repeated twice.
            address &= 0x0E00FFFF;

            switch(width) {
                case 32: {
                    value = ARM7TDMI::aluShiftRor(value, address * 8);
                    writeToArray8(&gamePakSram, address, 0x0E000000, value); 
                    break;
                }
                case 16: {
                    value = ARM7TDMI::aluShiftRor(value, address * 8);
                    writeToArray8(&gamePakSram, address, 0x0E000000, value); 
                    break;
                }
                case 8: {
                    writeToArray8(&gamePakSram, address, 0x0E000000, value); 
                    break;           
                }
                default: {
                    assert(false);
                    break;
                }
            } 

            break;
        }
        default: {
            // TODO: implement unused memory access behaviour (and check for unused memory writes)
            if(0x10000000 <= address && address <= 0xFFFFFFFF) {
                DEBUGWARN("writing to unused memory! address " << address << "\n");
            }            
            break;
        }
    }

    // if(address <= 0x00003FFF) {
    // } else if (0x02000000 <= address && address <= 0x02FFFFFF) {
    // } else if (0x03000000 <= address && address <= 0x03FFFFFF) {
    // } else if (0x04000000 <= address && address <= 0x040003FE) {
    //     if(0x04000000 <= address && address < 0x04000056) {
    //     }

    //     if(0x4000100 <= address && address <= 0x400010F) {
    //         // timer addresses
    //         timer->updateTimerUponWrite(address, value, width);
    //     }
    //     // DEBUGWARN("width " << (uint32_t)width << "\n");
    //     // DEBUGWARN("value " << value << "\n");
    //     // DEBUGWARN("addr " << address << "\n");

    //     switch(width) {
    //         case 32: {
    //             writeToArray32(&iORegisters, address, 0x04000000, value); 
    //             break;
    //         }
    //         case 16: {
    //             writeToArray16(&iORegisters, address, 0x04000000, value);      
    //             break;      
    //         }
    //         case 8: {
    //             writeToArray8(&iORegisters, address, 0x04000000, value);         
    //             break;  
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     } 

    //     // SPECIAL CASE when writing to interrupt request register
    //     // setting a bit (acknowledging an interrupt) changes that bit to zero
    //     // so do val &= ~val
    //     if(0x4000200 <= address && address <= 0x4000203) {
    //         //DEBUGWARN("eyo1\n");
    //         uint8_t tempWidth = width;
    //         uint32_t tempAddress = address;
    //         uint32_t tempValue = value;
    //         while(tempWidth != 0) {
    //             //DEBUGWARN("heyo\n");
    //             //DEBUGWARN("tempAddress " << tempAddress << "\n");
                
    //             if(tempAddress == 0x04000202 || tempAddress == 0x04000203) {
    //                 //DEBUGWARN("before " << (uint32_t)iORegisters[tempAddress - 0x04000000] << "\n");
    //                 iORegisters[tempAddress - 0x04000000] = iORegisters[tempAddress - 0x04000000] & (~tempValue);
    //                 //DEBUGWARN("after " << (uint32_t)iORegisters[tempAddress - 0x04000000] << "\n");
    //             }
                
    //             tempWidth -= 8;
    //             tempAddress += 1;
    //             tempValue = tempValue >> 8;
    //         }
    //         //DEBUGWARN("eyo2\n");
    //     }          

    // } else if (0x05000000 <= address && address <= 0x05FFFFFF) { 
    //     //DEBUGWARN("writing to palette ram: [" << address << "] = " << value << " " << (uint32_t)width << "\n");
    //     address &= 0x050003FF;
    //     switch(width) {
    //         case 32: {
    //             writeToArray32(&paletteRam, address, 0x05000000, value); 
    //             break;
    //         }
    //         case 16: {
    //             writeToArray16(&paletteRam, address, 0x05000000, value);         
    //             break;    
    //         }
    //         case 8: {
    //             /*
    //             Writes to BG (6000000h-600FFFFh) (or 6000000h-6013FFFh in Bitmap mode) 
    //             and to Palette (5000000h-50003FFh) are writing the new 8bit value to
    //              BOTH upper and lower 8bits of the addressed halfword, ie. "[addr AND NOT 1]=data*101h".
    //              */
    //             writeToArray16(&paletteRam, address & 0xFFFFFFFE, 0x05000000, value * 0x101); 
    //             break;            
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     } 

    // } else if (0x06000000 <= address && address <= 0x06FFFFFF) {    
    //     // DEBUGWARN("vram: writing " << value << " to " << address - 0x06000000 << "\n");
    //     // Even though VRAM is sized 96K (64K+32K), it is repeated in steps of 128K 
    //     // (64K+32K+32K, the two 32K blocks itself being mirrors of each other).
    //         if(address & 0x00010000) {
    //             address &= 0x06007FFF;
    //             address += 0x10000;
    //         } else {
    //             address &= 0x0600FFFF;
    //         }
    //     //address = (address & 0x0607FFF);
    //     switch(width) {
    //         case 32: {
    //             writeToArray32(&vRam, address, 0x06000000, value);
    //             break;
    //         }
    //         case 16: {
    //             writeToArray16(&vRam, address, 0x06000000, value);    
    //             break;        
    //         }
    //         case 8: {
    //             writeToArray8(&vRam, address, 0x06000000, value);      
    //             break;      
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     }

    // } else if (0x07000000 <= address && address <= 0x07FFFFFF) {    
    //     // TODO: there are more hblank rules to implement
    //     address &= 0x070003FF;
    //     switch(width) {
    //         case 32: {
    //             writeToArray32(&objAttributes, address, 0x07000000, value);
    //             break;
    //         }
    //         case 16: {
    //             writeToArray16(&objAttributes, address, 0x07000000, value);          
    //             break;  
    //         }
    //         case 8: {
    //             // 8 bit Writes to OBJ (6010000h-6017FFFh) (or 6014000h-6017FFFh in Bitmap mode) 
    //             // and to OAM (7000000h-70003FFh) are ignored, the memory content remains unchanged.
    //             // writeToArray8(&objAttributes, address, 0x07000000, value);            
    //             break;
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     }   

    // } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
    //     //  TODO: *** Separate timings for sequential, and non-sequential accesses.
    //     // waitstate 0

    //     if(address - 0x08000000 > gamePakRom.size()) {
    //         DEBUGWARN("bigger!\n");
    //         DEBUGWARN(address - 0x08000000 << "\n");
    //     }
    //     //DEBUGWARN("writing to gampak\n");

    //     switch(width) {
    //         case 32: {
    //             //writeToArray32(&gamePakRom, address, 0x08000000, value);
    //             break;
    //         }
    //         case 16: {
    //             //writeToArray16(&gamePakRom, address, 0x08000000, value);         
    //             break;   
    //         }
    //         case 8: {
    //             //writeToArray8(&gamePakRom, address, 0x08000000, value);          
    //             break;  
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     }   

    // } else if (0x0A000000 <= address && address <= 0x0BFFFFFF) {
    //     //  TODO: *** Separate timings for sequential, and non-sequential accesses.
    //     // waitstate 1
    //     //assert(false);
    //     switch(width) {
    //         case 32: {
    //             //writeToArray32(&gamePakRom, address, 0x0A000000, value);
    //             break;
    //         }
    //         case 16: {
    //             //writeToArray16(&gamePakRom, address, 0x0A000000, value);         
    //             break;   
    //         }
    //         case 8: {
    //             //writeToArray8(&gamePakRom, address, 0x0A000000, value);           
    //             break;
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     }   

    // } else if (0x0C000000 <= address && address <= 0x0DFFFFFF) {
    //     //  TODO: *** Separate timings for sequential, and non-sequential accesses.
    //     // waitstate 2
    //     //assert(false);
    //     switch(width) {
    //         case 32: {
    //             //writeToArray32(&gamePakRom, address, 0x0C000000, value);
    //             break;
    //         }
    //         case 16: {
    //             //writeToArray16(&gamePakRom, address, 0x0C000000, value);            
    //             break;
    //         }
    //         case 8: {
    //             //writeToArray8(&gamePakRom, address, 0x0C000000, value);            
    //             break;
    //         }
    //         default: {
    //             assert(false);
    //         }
    //     }   
    // } else if (0x0E000000 <= address && address <= 0xFFFFFFF) {
        // The 64K SRAM area is mirrored across the whole 32MB area at E000000h-FFFFFFFh, 
        // also, inside of the 64K SRAM field, 32K SRAM chips are repeated twice.
    //     address &= 0x0E00FFFF;

    //     switch(width) {
    //         case 32: {
    //             value = ARM7TDMI::aluShiftRor(value, address * 8);
    //             writeToArray8(&gamePakSram, address, 0x0E000000, value); 
    //             break;
    //         }
    //         case 16: {
    //             value = ARM7TDMI::aluShiftRor(value, address * 8);
    //             writeToArray8(&gamePakSram, address, 0x0E000000, value); 
    //             break;
    //         }
    //         case 8: {
    //             writeToArray8(&gamePakSram, address, 0x0E000000, value); 
    //             break;           
    //         }
    //         default: {
    //             assert(false);
    //             break;
    //         }
    //     } 
    // } 
}


void Bus::loadRom(std::vector<uint8_t> &buffer) {
    // TODO: assert that roms are smaller than 32MB
    for (int i = 0; i < buffer.size(); i++) {
        gamePakRom[i] = buffer[i];
    }
}


uint8_t Bus::getCurrentNWaitstate() {
    return currentNWaitstate;
}

uint8_t Bus::getCurrentSWaitstate() {
    return currentSWaitstate;
}


void Bus::write32(uint32_t address, uint32_t word, CycleType accessType) {
    write(address, word, 32, accessType);
}

uint32_t Bus::read32(uint32_t address, CycleType accessType) {
    return read(address, 32, accessType);
}

void Bus::write16(uint32_t address, uint16_t halfWord, CycleType accessType) {
    write(address, halfWord, 16, accessType);
}

uint16_t Bus::read16(uint32_t address, CycleType accessType) {
    return read(address, 16, accessType);
}

void Bus::write8(uint32_t address, uint8_t byte, CycleType accessType) {
    write(address, byte, 8, accessType);
}

uint8_t Bus::read8(uint32_t address, CycleType accessType) {
    return read(address, 8, accessType);
}


uint8_t Bus::getWaitState0NCycles() {
    return waitStateNVals[(iORegisters.at(waitcntOffset) & 0x0000000C) >> 2];
}
uint8_t Bus::getWaitState1NCycles() {
    return waitStateNVals[(iORegisters.at(waitcntOffset) & 0x00000060) >> 5];
}
uint8_t Bus::getWaitState2NCycles() {
    return waitStateNVals[(iORegisters.at(waitcntOffset) & 0x00000300) >> 8];
}

uint8_t Bus::getWaitState0SCycles() {
    return waitState0SVals[(iORegisters.at(waitcntOffset) & 0x00000010) >> 4];
}
uint8_t Bus::getWaitState1SCycles() {
    return waitState1SVals[(iORegisters.at(waitcntOffset) & 0x00000080) >> 7];
}
uint8_t Bus::getWaitState2SCycles() {
    return waitState2SVals[(iORegisters.at(waitcntOffset) & 0x00000400) >> 10];
}

void Bus::resetCycleCountTimeline() {
    //currentNWaitstate = 1;
    //currentSWaitstate = 1;
    executionTimelineSize = 0;
    memAccessCycles = 0;
}

uint32_t Bus::getMemoryAccessCycles() {
    return memAccessCycles;
}

void Bus::printCurrentExecutionTimeline() {
    std::cout << "[";
    for(int i = 0; i < executionTimelineSize; i++) {
        std::cout << "(" << cycleTypesSerialized[executionTimelineCycleType[i]] << ","
                         << (uint32_t)executionTimelineCycles[i] << "),";
    
    }
    std::cout << "]\n";
}

void Bus::connectTimer(Timer* _timer) {
    this->timer = _timer;
}

void Bus::connectDma(DMA* _dma) {
    this->dma = _dma;
}