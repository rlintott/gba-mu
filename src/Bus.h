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

    // 2kB placeholder random access memory for testing
    std::vector<uint8_t> ram;

    uint32_t read32(uint32_t address);
    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);

    uint32_t write32(uint32_t address, uint32_t word);
    uint32_t write8(uint32_t address, uint8_t byte);
    uint32_t write16(uint32_t address, uint16_t halfWord);

    void tempPushToRam(uint8_t byte);
    
};