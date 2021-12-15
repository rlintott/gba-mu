#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess
ARM7TDMI::thumbLdStSeBHwHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0x5000);
    assert(instruction & 0x0200);
    //uint8_t opcode = (instruction & 0x0C00) >> 10;
    constexpr uint8_t opcode = (op & 0x030) >> 4;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    //uint8_t ro = (instruction & 0x01C0) >> 6;
    constexpr uint8_t ro = (op & 0x007);
    uint32_t address = cpu->getRegister(rb) + cpu->getRegister(ro);

    Cycles cycles;

    if constexpr(opcode == 0) {
        // 0: STRH Rd,[Rb,Ro]  ;store 16bit data          HALFWORD[Rb+Ro] =
        // Rd
        cpu->bus->write16(address & 0xFFFFFFFE, cpu->getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
    } else if constexpr(opcode == 1) {
        // 1: LDSB Rd,[Rb,Ro]  ;load sign-extended 8bit   Rd = BYTE[Rb+Ro]
        uint32_t value = cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL);
        value = (value & 0x80) ? (0xFFFFFF00 | value) : value;
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);         
    } else if constexpr(opcode == 2) {
        // 2: LDRH Rd,[Rb,Ro]  ;load zero-extended 16bit  Rd =
        // HALFWORD[Rb+Ro]
        uint32_t value = aluShiftRor(cpu->bus->read16(address & 0xFFFFFFFE, Bus::CycleType::NONSEQUENTIAL),
                                        (address & 1) * 8);
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    } else {
        // 3: LDSH Rd,[Rb,Ro]  ;load sign-extended 16bit  Rd = HALFWORD[Rb+Ro]
        if(address & 0x1) {
            // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value
            // if address odd
            uint32_t value = cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL);
            value = (value & 0x80) ? (0xFFFFFF00 | value) : value;
            cpu->setRegister(rd, value);
        } else {
            uint32_t value = aluShiftRor(cpu->bus->read16(address & 0xFFFFFFFE, Bus::CycleType::NONSEQUENTIAL),
                                            (address & 1) * 8);
            value = (value & 0x8000) ? (0xFFFF0000 | value) : value;
            cpu->setRegister(rd, value);
        }
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0); 
    }

    return NONSEQUENTIAL;
}
