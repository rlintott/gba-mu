#pragma once

#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbAddSubHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF800) == 0x1800);
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rs = thumbGetRs(instruction);
    uint32_t op2;
    uint32_t result;
    uint32_t rsVal = cpu->getRegister(rs);

    bool carryFlag;
    bool overflowFlag;
    bool zeroFlag;
    bool signFlag;

    //uint8_t opcode = (instruction & 0x0600) >> 9;
    //01 1000
    constexpr uint8_t opcode = (op & 0x018) >> 3;

    if constexpr(opcode == 0) {
        // 0: ADD{S} Rd,Rs,Rn   ;add register Rd=Rs+Rn
        op2 = cpu->getRegister((instruction & 0x01C0) >> 6);
        result = rsVal + op2;
        carryFlag = aluAddSetsCarryBit(rsVal, op2);
        overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
    } else if constexpr(opcode == 1) {
        // 1: SUB{S} Rd,Rs,Rn   ;subtract register   Rd=Rs-Rn
        op2 = cpu->getRegister((instruction & 0x01C0) >> 6);
        result = rsVal - op2;
        carryFlag = aluSubtractSetsCarryBit(rsVal, op2);
        overflowFlag = aluSubtractSetsOverflowBit(rsVal, op2, result);
    } else if constexpr(opcode == 2) {
        // 2: ADD{S} Rd,Rs,#nn  ;add immediate       Rd=Rs+nn
        op2 = (instruction & 0x01C0) >> 6;
        result = rsVal + op2;
        carryFlag = aluAddSetsCarryBit(rsVal, op2);
        overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
    } else {
        // opcode == 3
        // 3: SUB{S} Rd,Rs,#nn  ;subtract immediate  Rd=Rs-nn
        op2 = (instruction & 0x01C0) >> 6;
        result = rsVal - op2;
        carryFlag = aluSubtractSetsCarryBit(rsVal, op2);
        overflowFlag = aluSubtractSetsOverflowBit(rsVal, op2, result);
    }
    signFlag = aluSetsSignBit(result);
    zeroFlag = aluSetsZeroBit(result);

    cpu->setRegister(rd, result);
    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.N = signFlag;
    cpu->cpsr.V = overflowFlag;
  
    return SEQUENTIAL;
}
