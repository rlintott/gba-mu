#pragma once

#include <vector>
#include <array>
#include "ARM7TDMI.h"

class Bus {

public:
    Bus();
    ~Bus();

public:

    // 2kB placeholder random access memory for testing
    std::array<uint32_t, 256> ram;

    // Gameboy Advance's ARM CPU
    ARM7TDMI cpu;

    uint32_t read(uint32_t address);

    uint32_t write(uint32_t address);
    
};