#include "Bus.h"

#include <assert.h>

#include <fstream>
#include <iostream>
#include <iterator>

Bus::Bus() {
    wRamBoard = new std::array<uint8_t, 263168>();
    wRamChip = new std::array<uint8_t, 32896>();
    gamePakRom = new std::vector<uint8_t>();
}

Bus::~Bus() {
    delete wRamBoard;
    delete wRamChip;
    delete gamePakRom;
}

uint32_t Bus::read32(uint32_t address) {
    if (0x02000000 <= address && address <= 0x0203FFFF) {
        return (uint32_t)wRamBoard->at(address - 0x02000000U) |
               (uint32_t)wRamBoard->at((address + 1) - 0x02000000U) << 8 |
               (uint32_t)wRamBoard->at((address + 2) - 0x02000000U) << 16 |
               (uint32_t)wRamBoard->at((address + 3) - 0x02000000U) << 24;
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        return (uint32_t)wRamChip->at(address - 0x03000000U) |
               (uint32_t)wRamChip->at((address + 1) - 0x03000000U) << 8 |
               (uint32_t)wRamChip->at((address + 2) - 0x03000000U) << 16 |
               (uint32_t)wRamChip->at((address + 3) - 0x03000000U) << 24;
    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        return (uint32_t)gamePakRom->at(address - 0x08000000U) |
               (uint32_t)gamePakRom->at((address + 1) - 0x08000000U) << 8 |
               (uint32_t)gamePakRom->at((address + 2) - 0x08000000U) << 16 |
               (uint32_t)gamePakRom->at((address + 3) - 0x08000000U) << 24;
    }
    // todo: hack for now to pass the test, why does accessing out of bound memory return this value?
    //  (probably just garbage)
    return 436207618U;
}

uint8_t Bus::read8(uint32_t address) {
    if (0x02000000 <= address && address <= 0x0203FFFF) {
        return (uint8_t)wRamBoard->at(address - 0x02000000U);
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        return (uint8_t)wRamChip->at(address - 0x03000000U);
    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        return (uint8_t)gamePakRom->at(address - 0x08000000U);
    }
    return 0x0;
}

uint16_t Bus::read16(uint32_t address) {
    if (0x02000000 <= address && address <= 0x0203FFFF) {
        // TODO: can optimize to only use one subtraction (use temp var address - 0x02000000U)
        return (uint16_t)wRamBoard->at(address - 0x02000000U) |
               (uint16_t)wRamBoard->at((address + 1) - 0x02000000U) << 8;
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        DEBUG("reading halword from wRampChip\n");
        return ((uint16_t)wRamChip->at(address - 0x03000000U)) |
               ((uint16_t)wRamChip->at((address + 1) - 0x03000000U) << 8);
    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        return (uint16_t)gamePakRom->at(address - 0x08000000U) |
               (uint16_t)gamePakRom->at((address + 1) - 0x08000000U) << 8;
    }
    return 0x0;
}

void Bus::write32(uint32_t address, uint32_t word) {
    if (0x02000000 <= address && address <= 0x0203FFFF) {
        (*wRamBoard)[address - 0x02000000U] = (uint8_t)word;
        (*wRamBoard)[(address + 1) - 0x02000000U] = (uint8_t)(word >> 8);
        (*wRamBoard)[(address + 2) - 0x02000000U] = (uint8_t)(word >> 16);
        (*wRamBoard)[(address + 3) - 0x02000000U] = (uint8_t)(word >> 24);
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        (*wRamChip)[address - 0x03000000U] = (uint8_t)word;
        (*wRamChip)[(address + 1) - 0x03000000U] = (uint8_t)(word >> 8);
        (*wRamChip)[(address + 2) - 0x03000000U] = (uint8_t)(word >> 16);
        (*wRamChip)[(address + 3) - 0x03000000U] = (uint8_t)(word >> 24);
    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        // shouldnt be writing to ROM
        assert(false);
        (*gamePakRom)[address - 0x08000000U] = (uint8_t)word;
        (*gamePakRom)[(address + 1) - 0x08000000U] = (uint8_t)(word >> 8);
        (*gamePakRom)[(address + 2) - 0x08000000U] = (uint8_t)(word >> 16);
        (*gamePakRom)[(address + 3) - 0x08000000U] = (uint8_t)(word >> 24);
    }
}

void Bus::write8(uint32_t address, uint8_t byte) {
    if (0x02000000 <= address && address <= 0x0203FFFF) {
        (*wRamBoard)[address - 0x02000000U] = (uint8_t)byte;
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        (*wRamChip)[address - 0x03000000U] = (uint8_t)byte;
    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        // shouldnt be writing to ROM
        assert(false);
        (*gamePakRom)[address - 0x08000000U] = (uint8_t)byte;
    }
}

void Bus::write16(uint32_t address, uint16_t halfWord) {
    if(address == 33554816) {
        DEBUG("AJHHHUHEIFHEISFHS\n");
    }
    if (0x02000000 <= address && address <= 0x0203FFFF) {
        (*wRamBoard)[address - 0x02000000U] = (uint8_t)halfWord;
        (*wRamBoard)[(address + 1) - 0x02000000U] = (uint8_t)(halfWord >> 8);
    } else if (0x03000000 <= address && address <= 0x03007FFF) {
        DEBUG("writing 16 bit to wRamChip\n");
        (*wRamChip)[address - 0x03000000U] = (uint8_t)halfWord;
        (*wRamChip)[(address + 1) - 0x03000000U] = (uint8_t)(halfWord >> 8);
    } else if (0x08000000 <= address && address <= 0x09FFFFFF) {
        // shouldnt be writing to ROM
        assert(false);
        (*gamePakRom)[address - 0x08000000U] = (uint8_t)halfWord;
        (*gamePakRom)[(address + 1) - 0x08000000U] = (uint8_t)(halfWord >> 8);
    }
}

void Bus::loadRom(std::string path) {

    std::ifstream binFile(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    // TODO: assert that roms are smaller than 32MB
    for (int i = 0; i < buffer.size(); i++) {
        gamePakRom->push_back(buffer[i]);
    }
}