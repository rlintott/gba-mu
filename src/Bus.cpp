#include "Bus.h"

#include <iostream>
#include <fstream>
#include <iterator>


Bus::Bus() {
    // allocating these large objects on the heap
    wRamBoard = new std::array<uint8_t, 263168>();
    wRamChip = new std::array<uint8_t, 32896>();
}

Bus::~Bus() {
    delete wRamBoard;
    delete wRamChip;
    delete gamePakRom;
}

uint32_t Bus::read32(uint32_t address) {
    if(0x02000000 <= address && address <= 0x0203FFFF) {
        return (uint32_t)wRamBoard->at(address)           |
               (uint32_t)wRamBoard->at(address + 1)  << 8 |
               (uint32_t)wRamBoard->at(address + 2) << 16 |
               (uint32_t)wRamBoard->at(address + 3) << 24;
    }
    else if(0x03000000 <= address && address <= 0x03007FFF) {
        return (uint32_t)wRamChip->at(address)           |
               (uint32_t)wRamChip->at(address + 1)  << 8 |
               (uint32_t)wRamChip->at(address + 2) << 16 |
               (uint32_t)wRamChip->at(address + 3) << 24;
    } 
    else if(0x08000000 <= address && address <= 0x09FFFFFF) {
        return (uint32_t)gamePakRom->at(address)           |
               (uint32_t)gamePakRom->at(address + 1)  << 8 |
               (uint32_t)gamePakRom->at(address + 2) << 16 |
               (uint32_t)gamePakRom->at(address + 3) << 24;
    }     
    return 0x0;
}


uint8_t Bus::read8(uint32_t address){

}
uint16_t Bus::read16(uint32_t address){

}

uint32_t Bus::write32(uint32_t address, uint32_t word){

}
uint32_t Bus::write8(uint32_t address, uint8_t byte){

}
uint32_t Bus::write16(uint32_t address, uint16_t halfWord){

}

void Bus::loadRom(std::string path) {
    std::ifstream binFile(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    // TODO: assert that roms are smaller than 32MB
    for(int i = 0; i < buffer.size(); i++) {
        gamePakRom->push_back(buffer[i]);
    }
}