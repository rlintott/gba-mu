#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbBHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF800) == 0xE000);
    uint32_t offset = signExtend12Bit((instruction & 0x07FF) << 1);
    cpu->setRegister(PC_REGISTER, (cpu->getRegister(PC_REGISTER) + 2 + offset) & 0xFFFFFFFE);
    return BRANCH; 
}
