#include "Bus.h"

Bus::Bus() {
}

Bus::~Bus() {
}

uint32_t Bus::read32(uint32_t address) {
    if(0x00 <= address && address < 0x800) {
        return ((uint32_t)0) | 
               (uint32_t)ram[address]           |
               (uint32_t)ram[address + 1]  << 8 |
               (uint32_t)ram[address + 2] << 16 |
               (uint32_t)ram[address + 3] << 24;
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


void Bus::tempPushToRam(uint8_t byte) {
    ram.push_back(byte);
    return;
}