#pragma once

#include <vector>
#include "ARM7TDMI.h"

class Bus {

public:
    Bus();
    ~Bus();

public:

    // Gameboy Advance's ARM CPU
    ARM7TDMI arm7tdmi;

    uint32_t read(uint32_t address);

    uint32_t write(uint32_t address);
    
};