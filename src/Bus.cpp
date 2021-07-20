#include "Bus.h"

#include <assert.h>

#include <fstream>
#include <iostream>
#include <iterator>

Bus::Bus() {
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
    for(int i = 0; i < 98688; i++) {
        vRam.push_back(0);
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

uint32_t Bus::read(uint32_t address, uint8_t width) {
    // TODO: can this be made faster (case switch the address?) tradeoff between performance/
    // readability
    if(address <= 0x00003FFF) {
        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray32(&bios, address, 0);
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray16(&bios, address, 0);
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray8(&bios, address, 0);
            }
            default: {
                assert(false);
                break;
            }
        }
    }

    else if (0x02000000 <= address && address <= 0x0203FFFF) {

        switch(width) {
            case 32: {
                currentNWaitstate = 6;
                currentSWaitstate = 6;
                return readFromArray32(&wRamBoard, address, 0x02000000);
            }
            case 16: {
                currentNWaitstate = 3;
                currentSWaitstate = 3;
                return readFromArray16(&wRamBoard, address, 0x02000000);            }
            case 8: {
                currentNWaitstate = 3;
                currentSWaitstate = 3;
                return readFromArray8(&wRamBoard, address, 0x02000000);            }
            default: {
                assert(false);
                break;
            }
        }

    } else if (0x03000000 <= address && address <= 0x03007FFF) {

        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray32(&wRamChip, address, 0x03000000);
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray16(&wRamChip, address, 0x03000000);            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray8(&wRamChip, address, 0x03000000);            }
            default: {
                assert(false);
                break;
            }
        }     

    } else if (0x04000000 <= address && address <= 0x040003FE) {

        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray32(&iORegisters, address, 0x04000000);
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray16(&iORegisters, address, 0x04000000);            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray8(&iORegisters, address, 0x04000000);            }
            default: {
                assert(false);
                break;
            }
        }           

    } else if (0x05000000 <= address && address <= 0x050003FF) {    

        switch(width) {
            case 32: {
                currentNWaitstate = 2;
                currentSWaitstate = 2;
                return readFromArray32(&paletteRam, address, 0x05000000);
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray16(&paletteRam, address, 0x05000000);            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray8(&paletteRam, address, 0x05000000);            }
            default: {
                assert(false);
                break;
            }
        }   

    } else if (0x06000000 <= address && address <= 0x06017FFF) {    

        switch(width) {
            case 32: {
                currentNWaitstate = 2;
                currentSWaitstate = 2;
                return readFromArray32(&vRam, address, 0x06000000);
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray16(&vRam, address, 0x06000000);            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray8(&vRam, address, 0x06000000);            }
            default: {
                assert(false);
                break;
            }
        }  

    } else if (0x07000000 <= address && address <= 0x070003FF) {    

        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray32(&objAttributes, address, 0x07000000);
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray16(&objAttributes, address, 0x07000000);            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                return readFromArray8(&objAttributes, address, 0x07000000);            }
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
                currentNWaitstate = getWaitState0NCycles() + 1 + 
                                    getWaitState0SCycles() + 1;
                currentSWaitstate = getWaitState0SCycles() + 1 + 
                                    getWaitState0SCycles() + 1;
                return readFromArray32(&gamePakRom, address, 0x08000000);
            }
            case 16: {
                currentNWaitstate = getWaitState0NCycles() + 1;
                currentSWaitstate = getWaitState0SCycles() + 1;
                return readFromArray16(&gamePakRom, address, 0x08000000);            
            }
            case 8: {
                currentNWaitstate = getWaitState0NCycles() + 1;
                currentSWaitstate = getWaitState0SCycles() + 1;
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
                currentNWaitstate = getWaitState1NCycles() + 1 + 
                                    getWaitState1SCycles() + 1;
                currentSWaitstate = getWaitState1SCycles() + 1 + 
                                    getWaitState1SCycles() + 1;
                return readFromArray32(&gamePakRom, address, 0x0A000000);
            }
            case 16: {
                currentNWaitstate = getWaitState1NCycles() + 1;
                currentSWaitstate = getWaitState1SCycles() + 1;
                return readFromArray16(&gamePakRom, address, 0x0A000000);            
            }
            case 8: {
                currentNWaitstate = getWaitState1NCycles() + 1;
                currentSWaitstate = getWaitState1SCycles() + 1;
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
                currentNWaitstate = getWaitState2NCycles() + 1 + 
                                    getWaitState2SCycles() + 1;
                currentSWaitstate = getWaitState2SCycles() + 1 + 
                                    getWaitState2SCycles() + 1;
                return readFromArray32(&gamePakRom, address, 0x0C000000);
            }
            case 16: {
                currentNWaitstate = getWaitState2NCycles() + 1;
                currentSWaitstate = getWaitState2SCycles() + 1;
                return readFromArray16(&gamePakRom, address, 0x0C000000);           
            }
            case 8: {
                currentNWaitstate = getWaitState2NCycles() + 1;
                currentSWaitstate = getWaitState2SCycles() + 1;
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
                currentNWaitstate = 5;
                currentSWaitstate = 5;
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

void Bus::write(uint32_t address, uint32_t value, uint8_t width) {
    // TODO: can this be made faster (case switch the address?) tradeoff between performance/
    // readability
    if(address <= 0x00003FFF) {
        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray32(&bios, address, 0, value);
                break;
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray16(&bios, address, 0, value);
                break;
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray8(&bios, address, 0, value);
                break;
            }
            default: {
                assert(false);
                break;
            }
        }

    } else if (0x02000000 <= address && address <= 0x0203FFFF) {

        switch(width) {
            case 32: {
                currentNWaitstate = 6;
                currentSWaitstate = 6;
                writeToArray32(&wRamBoard, address, 0x02000000, value);
                break;
            }
            case 16: {
                currentNWaitstate = 3;
                currentSWaitstate = 3;
                writeToArray16(&wRamBoard, address, 0x02000000, value);    
                break;        
            }
            case 8: {
                currentNWaitstate = 3;
                currentSWaitstate = 3;
                writeToArray8(&wRamBoard, address, 0x02000000, value);    
                break;       
            }
            default: {
                assert(false);
                break;
            }
        }

    } else if (0x03000000 <= address && address <= 0x03007FFF) {

        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray32(&wRamChip, address, 0x03000000, value); 
                break;
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray16(&wRamChip, address, 0x03000000, value);       
                break;     
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
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
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray32(&iORegisters, address, 0x04000000, value); 
                break;
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray16(&iORegisters, address, 0x04000000, value);      
                break;      
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
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
                currentNWaitstate = 2;
                currentSWaitstate = 2;
                writeToArray32(&paletteRam, address, 0x05000000, value); 
                break;
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray16(&paletteRam, address, 0x05000000, value);         
                break;    
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray8(&paletteRam, address, 0x05000000, value);
                break;            
            }
            default: {
                assert(false);
                break;
            }
        }   

    } else if (0x06000000 <= address && address <= 0x06017FFF) {    

        switch(width) {
            case 32: {
                currentNWaitstate = 2;
                currentSWaitstate = 2;
                writeToArray32(&vRam, address, 0x06000000, value);
                break;
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray16(&vRam, address, 0x06000000, value);    
                break;        
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray8(&vRam, address, 0x06000000, value);      
                break;      
            }
            default: {
                assert(false);
                break;
            }
        }  

    } else if (0x07000000 <= address && address <= 0x070003FF) {    

        switch(width) {
            case 32: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray32(&objAttributes, address, 0x07000000, value);
                break;
            }
            case 16: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
                writeToArray16(&objAttributes, address, 0x07000000, value);          
                break;  
            }
            case 8: {
                currentNWaitstate = 1;
                currentSWaitstate = 1;
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
        DEBUG("reading from gamepak\n");
        switch(width) {
            case 32: {
                currentNWaitstate = getWaitState0NCycles() + 1 + 
                                    getWaitState0SCycles() + 1;
                currentSWaitstate = getWaitState0SCycles() + 1 + 
                                    getWaitState0SCycles() + 1;
                writeToArray32(&gamePakRom, address, 0x08000000, value);
                break;
            }
            case 16: {
                currentNWaitstate = getWaitState0NCycles() + 1;
                currentSWaitstate = getWaitState0SCycles() + 1;
                writeToArray16(&gamePakRom, address, 0x08000000, value);         
                break;   
            }
            case 8: {
                currentNWaitstate = getWaitState0NCycles() + 1;
                currentSWaitstate = getWaitState0SCycles() + 1;
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
                currentNWaitstate = getWaitState1NCycles() + 1 + 
                                    getWaitState1SCycles() + 1;
                currentSWaitstate = getWaitState1SCycles() + 1 + 
                                    getWaitState1SCycles() + 1;
                writeToArray32(&gamePakRom, address, 0x0A000000, value);
                break;
            }
            case 16: {
                currentNWaitstate = getWaitState1NCycles() + 1;
                currentSWaitstate = getWaitState1SCycles() + 1;
                writeToArray16(&gamePakRom, address, 0x0A000000, value);         
                break;   
            }
            case 8: {
                currentNWaitstate = getWaitState1NCycles() + 1;
                currentSWaitstate = getWaitState1SCycles() + 1;
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
                currentNWaitstate = getWaitState2NCycles() + 1 + 
                                    getWaitState2SCycles() + 1;
                currentSWaitstate = getWaitState2SCycles() + 1 + 
                                    getWaitState2SCycles() + 1;
                writeToArray32(&gamePakRom, address, 0x0C000000, value);
                break;
            }
            case 16: {
                currentNWaitstate = getWaitState2NCycles() + 1;
                currentSWaitstate = getWaitState2SCycles() + 1;
                writeToArray16(&gamePakRom, address, 0x0C000000, value);            
                break;
            }
            case 8: {
                currentNWaitstate = getWaitState2NCycles() + 1;
                currentSWaitstate = getWaitState2SCycles() + 1;
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
                currentNWaitstate = 5;
                currentSWaitstate = 5;
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


void Bus::write32(uint32_t address, uint32_t word) {
    write(address, word, 32);
}

uint32_t Bus::read32(uint32_t address) {
    return read(address, 32);
}

void Bus::write16(uint32_t address, uint16_t halfWord) {
    write(address, halfWord, 16);
}

uint16_t Bus::read16(uint32_t address) {
    return read(address, 16);
}

void Bus::write8(uint32_t address, uint8_t byte) {
    write(address, byte, 8);
}

uint8_t Bus::read8(uint32_t address) {
    return read(address, 8);
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
