#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbLdStImmOffHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xE000) == 0x6000);
    // uint8_t opcode = (instruction & 0x1800) >> 11;
    constexpr uint8_t opcode = (op & 0x060) >> 5;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);

    if constexpr(opcode == 0) {
        // 0: STR  Rd,[Rb,#nn]  ;store 32bit data   WORD[Rb+nn] = Rd
        uint32_t offset = (instruction & 0x07C0) >> 4;
        uint32_t address = cpu->getRegister(rb) + offset;
        cpu->bus->write32(address, cpu->getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
    } else if constexpr(opcode == 1) {
        // 1: LDR  Rd,[Rb,#nn]  ;load  32bit data   Rd = WORD[Rb+nn]
        uint32_t offset = (instruction & 0x07C0) >> 4;
        uint32_t address = cpu->getRegister(rb) + offset;
        uint32_t value = aluShiftRor(cpu->bus->read32(address, 
                                                      Bus::CycleType::NONSEQUENTIAL),
                                     (address & 3) * 8);
        // if(rd == PC_REGISTER) {
        //     // For THUMB code, the low bit of the target address may/should/must be set,
        //     // the bit is (or is not) interpreted as thumb-bit (depending on the opcode), 
        //     // and R15 is then forcibly aligned by clearing the lower bit.
        //     // TODO: check that this is applied everywhere
        //     cpu->setRegister(rd, value & 0xFFFFFFFE);
        // } else {
        //     cpu->setRegister(rd, value);
        // }
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    } else if constexpr(opcode == 2) {
        // 2: STRB Rd,[Rb,#nn]  ;store  8bit data   BYTE[Rb+nn] = Rd
        uint32_t offset = (instruction & 0x07C0) >> 6;
        uint32_t address = cpu->getRegister(rb) + offset;
        cpu->bus->write8(address, (uint8_t)(cpu->getRegister(rd)), Bus::CycleType::NONSEQUENTIAL);
    } else {
        // 3: LDRB Rd,[Rb,#nn]  ;load   8bit data   Rd = BYTE[Rb+nn]
        uint32_t offset = (instruction & 0x07C0) >> 6;
        uint32_t address = cpu->getRegister(rb) + offset;
        cpu->setRegister(rd, (uint32_t)cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL));
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }

    return NONSEQUENTIAL;
}
