#include "Bus.h"

Bus::Bus() {
    cpu.connectBus(this);
}

uint32_t Bus::read(uint32_t address) {
    if(0x00 <= address && address < 0xFF) {
        return ram[address];
    }
    return 0x0;
}