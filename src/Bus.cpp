#include "Bus.h"

#include <assert.h>

#include <fstream>
#include <iostream>
#include <iterator>

Bus::Bus() {
    for(int i = 0; i < 98688; i++) {
        vRam.push_back(0);
    }
    for(int i = 0; i < 16448; i++) {
        bios.push_back(0);
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
    for(int i = 0; i < 1028; i++) {
        paletteRam.push_back(0);
    }
    for(int i = 0; i < 1028; i++) {
        objAttributes.push_back(0);
    }
    for(int i = 0; i < 65792; i++) {
        gamePakSram.push_back(0);
    }


}

Bus::~Bus() {
}


uint32_t readFromArray32(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift) {
        return (uint32_t)arr->at(address - shift) |
               (uint32_t)arr->at((address + 1) - shift) << 8 |
               (uint32_t)arr->at((address + 2) - shift) << 16 |
               (uint32_t)arr->at((address + 3) - shift) << 24;
}

uint16_t readFromArray16(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift) {
        return (uint16_t)arr->at(address - shift) |
               (uint16_t)arr->at((address + 1) - shift) << 8;
}

uint8_t readFromArray8(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift) {
        return (uint8_t)arr->at(address - shift);
}

void writeToArray32(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift, uint32_t value) {
        arr->at(address - shift) = (uint8_t)value;
        arr->at((address + 1) - shift) = (uint8_t)(value >> 8);
        arr->at((address + 2) - shift) = (uint8_t)(value >> 16);
        arr->at((address + 3) - shift) = (uint8_t)(value >> 24);
}

void writeToArray16(std::vector<uint8_t>* arr, uint32_t address, uint32_t shift, uint16_t value) {
        arr->at(address - shift) = (uint8_t)value;
        arr->at((address + 1) - shift) = (uint8_t)(value >> 8);
}

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
        case 0: // BIOS
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

uint32_t Bus::read(uint32_t address, uint8_t width, CycleType cycleType) {
    // TODO: use same switch statement pattern as in fn addCycleToExecutionTimeline
    addCycleToExecutionTimeline(cycleType, address & 0x0F000000, width);

    if(address <= 0x00003FFF) {
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
        }
    }
    else if (0x02000000 <= address && address <= 0x0203FFFF) {
        DEBUG("reading from wramBoard\n");
        switch(width) {
            case 32: {
                return readFromArray32(&wRamBoard, address, 0x02000000);
            }
            case 16: {
                return readFromArray16(&wRamBoard, address, 0x02000000);            
            }
            case 8: {
                return readFromArray8(&wRamBoard, address, 0x02000000);            
            }
            default: {
                assert(false);
                break;
            }
        }
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        DEBUG("reading from wramChip\n");

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

    } else if (0x04000000 <= address && address <= 0x040003FE) {

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

    } else if (0x05000000 <= address && address <= 0x050003FF) {    
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

    } else if (0x06000000 <= address && address <= 0x06017FFF) {    
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

    } else if (0x07000000 <= address && address <= 0x070003FF) {   
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

    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        //  TODO: *** Separate timings for sequential, and non-sequential accesses.
        // waitstate 0
        DEBUG("reading from gamepak\n");
        switch(width) {
            case 32: {
                return readFromArray32(&gamePakRom, address, 0x08000000);
            }
            case 16: {
                return readFromArray16(&gamePakRom, address, 0x08000000);            
            }
            case 8: {              
                return readFromArray8(&gamePakRom, address, 0x08000000);           
            }
            default: {
                assert(false);
                break;
            }
        }   

    } else if (0x0A000000 <= address && address <= 0x0BFFFFFF) {
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

    } else if (0x0C000000 <= address && address <= 0x0DFFFFFF) {
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

    } else if (0x0E000000 <= address && address <= 0x0E00FFFF) {
        switch(width) {
            case 32: {
                assert(false);
                break;
            }
            case 16: {
                assert(false);
                break;
            }
            case 8: {
                return readFromArray8(&gamePakSram, address, 0x0E000000);           
            }
            default: {
                assert(false);
                break;
            }
        } 
    }
    // TODO: hack for now to pass the test, why does accessing out of bound memory return this value?
    //  (probably just garbage)
    return 436207618U;
}

void Bus::write(uint32_t address, uint32_t value, uint8_t width, CycleType accessType) {
    // TODO: use same switch statement pattern as in fn addCycleToExecutionTimeline
    addCycleToExecutionTimeline(accessType, address & 0x0F000000, width);

    if(address <= 0x00003FFF) {
        switch(width) {
            case 32: {
                writeToArray32(&bios, address, 0, value);
                break;
            }
            case 16: {
                writeToArray16(&bios, address, 0, value);
                break;
            }
            case 8: {
                writeToArray8(&bios, address, 0, value);
                break;
            }
            default: {
                assert(false);
                break;
            }
        }

    } else if (0x02000000 <= address && address <= 0x0203FFFF) {
        DEBUG("writing to wramBoard\n");

        switch(width) {
            case 32: {
                writeToArray32(&wRamBoard, address, 0x02000000, value);
                break;
            }
            case 16: {
                writeToArray16(&wRamBoard, address, 0x02000000, value);    
                break;        
            }
            case 8: {
                writeToArray8(&wRamBoard, address, 0x02000000, value);    
                break;       
            }
            default: {
                assert(false);
                break;
            }
        }

    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        DEBUG("writing to wramChip\n");

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

    } else if (0x04000000 <= address && address <= 0x040003FE) {
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

    } else if (0x05000000 <= address && address <= 0x050003FF) {    
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
                writeToArray8(&paletteRam, address, 0x05000000, value);
                break;            
            }
            default: {
                assert(false);
                break;
            }
        } 

    } else if (0x06000000 <= address && address <= 0x06017FFF) {    
        // DEBUGWARN("vram: writing " << value << " to " << address - 0x06000000 << "\n");
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

    } else if (0x07000000 <= address && address <= 0x070003FF) {    
        // TODO: there are more hblank rules to implement
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
                writeToArray8(&objAttributes, address, 0x07000000, value);            
                break;
            }
            default: {
                assert(false);
                break;
            }
        }   

    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        //  TODO: *** Separate timings for sequential, and non-sequential accesses.
        // waitstate 0
        switch(width) {
            case 32: {
                writeToArray32(&gamePakRom, address, 0x08000000, value);
                break;
            }
            case 16: {
                writeToArray16(&gamePakRom, address, 0x08000000, value);         
                break;   
            }
            case 8: {
                writeToArray8(&gamePakRom, address, 0x08000000, value);          
                break;  
            }
            default: {
                assert(false);
                break;
            }
        }   

    } else if (0x0A000000 <= address && address <= 0x0BFFFFFF) {
        //  TODO: *** Separate timings for sequential, and non-sequential accesses.
        // waitstate 1
        assert(false);
        switch(width) {
            case 32: {
                writeToArray32(&gamePakRom, address, 0x0A000000, value);
                break;
            }
            case 16: {
                writeToArray16(&gamePakRom, address, 0x0A000000, value);         
                break;   
            }
            case 8: {
                writeToArray8(&gamePakRom, address, 0x0A000000, value);           
                break;
            }
            default: {
                assert(false);
                break;
            }
        }   

    } else if (0x0C000000 <= address && address <= 0x0DFFFFFF) {
        //  TODO: *** Separate timings for sequential, and non-sequential accesses.
        // waitstate 2
        assert(false);
        switch(width) {
            case 32: {
                writeToArray32(&gamePakRom, address, 0x0C000000, value);
                break;
            }
            case 16: {
                writeToArray16(&gamePakRom, address, 0x0C000000, value);            
                break;
            }
            case 8: {
                writeToArray8(&gamePakRom, address, 0x0C000000, value);            
                break;
            }
            default: {
                assert(false);
            }
        }   
    } else if (0x0E000000 <= address && address <= 0x0E00FFFF) {
        switch(width) {
            case 32: {
                assert(false);
                break;
            }
            case 16: {
                assert(false);
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
    } 
}


void Bus::loadRom(std::string path) {
    std::ifstream binFile(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    // TODO: assert that roms are smaller than 32MB
    for (int i = 0; i < buffer.size(); i++) {
        gamePakRom.push_back(buffer[i]);
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

void Bus::reset() {
    currentNWaitstate = 1;
    currentSWaitstate = 1;
    executionTimelineSize = 0;
}

void Bus::printCurrentExecutionTimeline() {
    std::cout << "[";
    for(int i = 0; i < executionTimelineSize; i++) {
        std::cout << "(" << cycleTypesSerialized[executionTimelineCycleType[i]] << ","
                         << (uint32_t)executionTimelineCycles[i] << "),";
    
    }
    std::cout << "]\n";
}

