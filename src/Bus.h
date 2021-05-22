#pragma once

// define ndebug to suppress debug logs  
// #define NDEBUG 1;

#include <vector>
#include <array>
#include "ARM7TDMI.h"

#ifdef NDEBUG 
#define DEBUG(x) 
#else
#define DEBUG(x) do { std::cerr << x; } while (0)
#endif


class Bus {

public:
    Bus();
    ~Bus();

public:

    // work ram! 02000000-0203FFFF (256kB)
    std::array<uint8_t, 263168>* wRamBoard;
    // 03000000-03007FFF (32 kB)
    std::array<uint8_t, 32896>* wRamChip;


    // Game Pak ROM/FlashROM (max 32MB), 08000000-09FFFFFF
    std::vector<uint8_t>* gamePakRom;

    uint32_t read32(uint32_t address);
    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);

    uint32_t write32(uint32_t address, uint32_t word);
    uint32_t write8(uint32_t address, uint8_t byte);
    uint32_t write16(uint32_t address, uint16_t halfWord);

    void loadRom(std::string path);

    
};