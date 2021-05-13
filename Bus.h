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
    std::array<uint8_t, 2048> ram;

    // Gameboy Advance's ARM CPU
    ARM7TDMI cpu;

    uint32_t read32(uint32_t address);

    void write8(uint32_t address, uint8_t byte);
    
};