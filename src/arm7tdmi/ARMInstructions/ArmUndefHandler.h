
#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"
#include <bitset>

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armUndefHandler(uint32_t instruction, ARM7TDMI* cpu) {
    DEBUGWARN("UNDEFINED ARM OPCODE! " << std::bitset<32>(instruction).to_string() << std::endl);
    cpu->switchToMode(ARM7TDMI::Mode::UNDEFINED);
    *(cpu->currentSpsr) = cpu->cpsr;
    cpu->cpsr.Mode = Mode::UNDEFINED;
    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return BRANCH;
}