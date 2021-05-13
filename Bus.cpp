#include "Bus.h"

Bus::Bus() {
    cpu.connectBus(this);
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


void Bus::write8(uint32_t address, uint8_t byte) {
    if(0x00 <= address && address < 0x7FF) {
        ram[address] = byte;
        return;
    }

    DEBUG("writing to non-existent memory location... " << byte);
}