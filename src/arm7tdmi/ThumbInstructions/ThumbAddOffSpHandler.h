#pragma once

#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbAddOffSpHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xFF00) == 0xB000);
    constexpr bool opcode = (op & 0x002);
    //uint8_t opcode = (instruction & 0x0080) >> 7;
    uint16_t offset = (instruction & 0x007F) << 2;

    if constexpr(!opcode) {
        // 0: ADD  SP,#nn       ;SP = SP + nn
        uint32_t value = cpu->getRegister(SP_REGISTER) + offset;
        cpu->setRegister(SP_REGISTER, value);
    } else {
        // 1: ADD  SP,#-nn      ;SP = SP - nn
        uint32_t value = cpu->getRegister(SP_REGISTER) - offset;
        cpu->setRegister(SP_REGISTER, value);
    }

    return SEQUENTIAL;
}
