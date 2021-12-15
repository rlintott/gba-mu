#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbImmHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xE000) == 0x2000);
    DEBUG("in thumb compare/add/subtract immediate\n");
    
    // uint8_t opcode = (instruction & 0x1800) >> 11;
    constexpr uint8_t opcode = (op & 0x060) >> 5;

    uint32_t offset = instruction & 0x00FF;
    //uint8_t rd = (instruction & 0x0700) >> 8;
    constexpr uint8_t rd = (op & 0x01C) >> 2;

    DEBUG("opcode: " << (uint32_t)opcode << "\n");
    DEBUG("offset: " << (uint32_t)offset << "\n");
    DEBUG("rd: " << (uint32_t)rd << "\n");

    bool carryFlag;
    bool signFlag;
    bool overflowFlag;
    bool zeroFlag;

    if constexpr(opcode == 0) {
        // 00b: MOV{S} Rd,#nn      ;move     Rd   = #nn
        cpu->setRegister(rd, offset);
        signFlag = aluSetsSignBit(offset);
        zeroFlag = aluSetsZeroBit(offset);
        carryFlag = cpu->cpsr.C;
        overflowFlag = cpu->cpsr.V;
    } else if constexpr(opcode == 1) {
        // 01b: CMP{S} Rd,#nn      ;compare  Void = Rd - #nn
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result = rdVal - offset;
        DEBUG("result: " << result << "\n");
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
        overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
    } else if constexpr(opcode == 2) {
        // 10b: ADD{S} Rd,#nn      ;add      Rd   = Rd + #nn
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result = rdVal + offset;
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        carryFlag = aluAddSetsCarryBit(rdVal, offset);
        overflowFlag = aluAddSetsOverflowBit(rdVal, offset, result);
        cpu->setRegister(rd, result);   
    } else {
        // opcode == 3
        // 11b: SUB{S} Rd,#nn      ;subtract Rd   = Rd - #nn
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result = rdVal - offset;
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
        overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
        cpu->setRegister(rd, result);
    }

    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.N = signFlag;
    cpu->cpsr.V = overflowFlag;

    return SEQUENTIAL;
}
