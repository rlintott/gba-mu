#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbLdStSpRelHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0x9000);
    //uint8_t opcode = (instruction & 0x0800) >> 11;
    constexpr bool opcode = (op & 0x020);
    //uint8_t rd = (instruction & 0x0700) >> 8;
    constexpr uint8_t rd = (op & 0x01C) >> 2;
    uint16_t offset = (instruction & 0x00FF) << 2;
    uint32_t address = cpu->getRegister(SP_REGISTER) + offset;

    if constexpr(!opcode) {
        // 0: STR  Rd,[SP,#nn]  ;store 32bit data   WORD[SP+nn] = Rd
        cpu->bus->write32(address & 0xFFFFFFFC, cpu->getRegister(rd), Bus::CycleType::NONSEQUENTIAL); 
    } else {
        // 1: LDR  Rd,[SP,#nn]  ;load  32bit data   Rd = WORD[SP+nn]
        uint32_t value = aluShiftRor(cpu->bus->read32(address & 0xFFFFFFFC, 
                                        Bus::CycleType::NONSEQUENTIAL),
                                        (address & 3) * 8);
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }

    return NONSEQUENTIAL;
}

