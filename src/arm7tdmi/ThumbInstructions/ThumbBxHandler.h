#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbBxHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xFC00) == 0x4400);
    //uint8_t opcode = (instruction & 0x0300) >> 8;
    constexpr uint8_t opcode = (op & 0x00C) >> 2;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    //uint8_t msbd = (instruction & 0x0080) >> 7;  // MSBd - Destination Register most significant bit (or BL/BLX flag)
    //uint8_t msbs = (instruction & 0x0040) >> 6;  // MSBs - Source Register most significant bit
    constexpr bool msbd = op & 0x002;
    constexpr bool msbs = op & 0x001;

    if constexpr(msbs) {
        rs = rs | 0x8;
    } 
    if constexpr(msbd) {
        rd = rd | 0x8;
    } 

    uint32_t rsVal = cpu->getRegister(rs);
    uint32_t rdVal = cpu->getRegister(rd);
    rsVal = (rs == PC_REGISTER) ? (rsVal + 2) & 0xFFFFFFFE : rsVal;
    rdVal = (rd == PC_REGISTER) ? (rdVal + 2) & 0xFFFFFFFE : rdVal;

    // TODO: why do we have to do this even if rs != 15? 
    if(rd == PC_REGISTER) {
        rsVal &= 0xFFFFFFFE;
    }

    if constexpr(opcode == 0) {
        // 0: ADD Rd,Rs   ;add        Rd = Rd+Rs
        assert(msbd || msbs);
        uint32_t result = rdVal + rsVal;
        cpu->setRegister(rd, result);
        if(rd == PC_REGISTER) {
            return BRANCH;
        } else {
            return SEQUENTIAL;
        }
    } else if constexpr(opcode == 1) {
        // 1: CMP Rd,Rs   ;compare  Void = Rd-Rs  ;cpu->cpsr affected
        uint32_t result;
        result = rdVal - rsVal;
        cpu->cpsr.N = aluSetsSignBit((uint32_t)result);
        cpu->cpsr.Z = aluSetsZeroBit((uint32_t)result);
        cpu->cpsr.C = aluSubtractSetsCarryBit(rdVal, rsVal);
        cpu->cpsr.V = aluSubtractSetsOverflowBit(rdVal, rsVal, result);
        return BRANCH;
    } else if constexpr(opcode == 2) {
        // 2: MOV Rd,Rs   ;move       Rd = Rs
        cpu->setRegister(rd, rsVal);
        assert(msbd || msbs);
        if(rd == PC_REGISTER) {
            return BRANCH;
        } else {
            return SEQUENTIAL;
        }    
    } else {
        // opcode == 3
        // 3: BX  Rs      ;jump       PC = Rs     ;may switch THUMB/ARM
        if (rs == PC_REGISTER || !(rsVal & 0x1)) {
            // R15: CPU switches to ARM state, and PC is auto-aligned as
            // (($+4) AND NOT 2).
            // For BX/BLX, when Bit 0 of the value in Rs is zero:
            // Processor will be switched into ARM mode!
            // If so, Bit 1 of Rs must be cleared (32bit word aligned).
            // Thus, BX PC (switch to ARM) may be issued from word-aligned
            // address only, the destination is PC+4 (ie. the following
            // halfword is skipped).
            cpu->cpsr.T = 0;
            rsVal &= 0xFFFFFFFD;
        }
        assert(msbd == 0);
        rsVal &= 0xFFFFFFFE;
        cpu->setRegister(PC_REGISTER, rsVal);

        return BRANCH; 
    }

}
