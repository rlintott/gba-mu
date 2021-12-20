#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbLdStHwHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0x8000);
    //uint8_t opcode = (instruction & 0x0800) >> 11;
    constexpr bool opcode = (op & 0x020);
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    //uint32_t offset = (instruction & 0x07C0) >> 5;
    constexpr uint32_t offset = (op & 0x01F) << 1;
    uint32_t address = cpu->getRegister(rb) + offset;

    if constexpr(!opcode) {
        // 0: STRH Rd,[Rb,#nn]  ;store 16bit data   HALFWORD[Rb+nn] = Rd
        cpu->bus->write16(address,
                     (uint16_t)cpu->getRegister(rd), 
                     Bus::CycleType::NONSEQUENTIAL);
    } else {
        // 1: LDRH Rd,[Rb,#nn]  ;load  16bit data   Rd = HALFWORD[Rb+nn]
        uint32_t value = aluShiftRor(cpu->bus->read16(address, 
                                     Bus::CycleType::NONSEQUENTIAL),
                                     (address & 1) * 8);
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }

    return NONSEQUENTIAL;
}
