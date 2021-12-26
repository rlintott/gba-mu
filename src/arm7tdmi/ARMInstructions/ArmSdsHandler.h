
#pragma once
#include "../ARM7TDMI.h"
#include "../memory/Bus.h"


template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armSdsHandler(uint32_t instruction, ARM7TDMI* cpu) {
    // TODO: figure out memory alignment logic (for all data transfer ops)
    // (verify against existing CPU implementations
    constexpr bool b = op & 0x040;
    assert((instruction & 0x0F800000) == 0x01000000);
    assert(!(instruction & 0x00300000));
    assert((instruction & 0x00000FF0) == 0x00000090);
    uint8_t rn = getRn(instruction);
    uint8_t rd = getRd(instruction);
    uint8_t rm = getRm(instruction);
    assert((rn != 15) && (rd != 15) && (rm != 15));

    // SWP{cond}{B} Rd,Rm,[Rn]     ;Rd=[Rn], [Rn]=Rm
    if constexpr(b) {
        // SWPB swap byte
        uint32_t rnVal = cpu->getRegister(rn);
        uint8_t rmVal = (uint8_t)(cpu->getRegister(rm));
        uint8_t data = cpu->bus->read8(rnVal, Bus::CycleType::NONSEQUENTIAL);
        cpu->setRegister(rd, (uint32_t)data);
        // TODO: check, is it a zero-extended 32-bit write or an 8-bit write?
        cpu->bus->write8(rnVal, rmVal, Bus::CycleType::NONSEQUENTIAL);
    } else {
        // SWPB swap word
        // The SWP opcode works like a combination of LDR and STR, that means, 
        // it does read-rotated, but does write-unrotated.
        uint32_t rnVal = cpu->getRegister(rn);
        uint32_t rmVal = cpu->getRegister(rm);
        
        uint32_t data = aluShiftRor(cpu->bus->read32(rnVal, Bus::CycleType::NONSEQUENTIAL), (rnVal & 3) * 8);
        cpu->setRegister(rd, data);
        cpu->bus->write32(rnVal, rmVal, Bus::CycleType::NONSEQUENTIAL);
    }
    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return NONSEQUENTIAL;

}