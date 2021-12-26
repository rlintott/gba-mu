#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"


template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbUndefHandler(uint16_t instruction, ARM7TDMI* cpu) {
    DEBUGWARN("UNDEFINED THUMB OPCODE! " << std::bitset<16>(instruction).to_string() << std::endl);
    
    // cpu->switchToMode(ARM7TDMI::Mode::UNDEFINED);
    // TODO: what is behaviour of thumb undefined instruction?
    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return BRANCH;
}
