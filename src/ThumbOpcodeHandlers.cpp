#include <bitset>

#include "ARM7TDMI.h"
#include "Bus.h"
#include "assert.h"

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::shiftHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xE000) == 0);

    uint8_t opcode = (instruction & 0x1800) >> 11;
    uint8_t offset = (instruction & 0x07C0) >> 6;
    // TODO: make into common fn
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);

    uint32_t rsVal = cpu->getRegister(rs);

    bool carryFlag;
    uint32_t result;

    switch (opcode) {
        case 0: {
            // 00b: LSL{S} Rd,Rs,#Offset (logical/arithmetic shift left)
            if (offset == 0) {
                result = rsVal;
                carryFlag = cpu->cpsr.C;
            } else {
                result = cpu->aluShiftLsl(rsVal, offset);
                // TODO: put the logic for shift cary bit into functions
                carryFlag = (rsVal >> (32 - offset)) & 1;
            }
            break;
        }
        case 1: {
            // 01b: LSR{S} Rd,Rs,#Offset (logical shift right)
            if (offset == 0) {
                result = cpu->aluShiftLsr(rsVal, 32);
            } else {
                result = cpu->aluShiftLsr(rsVal, offset);
            }
            carryFlag = (rsVal >> (offset - 1)) & 1;
            break;
        }
        case 2: {
            // 10b: ASR{S} Rd,Rs,#Offset (arithmetic shift right)
            if (offset == 0) {
                result = cpu->aluShiftAsr(rsVal, 32);
            } else {
                result = cpu->aluShiftAsr(rsVal, offset);
            }
            carryFlag = (rsVal >> (offset - 1)) & 1;
            break;
        }
        case 3: {
            // 11b: Reserved (used for add/subtract instructions)
            assert(false);
            break;
        }

            cpu->setRegister(rd, result);

            cpu->cpsr.C = carryFlag;
            cpu->cpsr.Z = (result == 0);
            cpu->cpsr.N = (result & 0x80000000);
    }
    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::addSubHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
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

    uint8_t opcode = (instruction & 0x0600) >> 9;

    switch(opcode) {
        case 0: {
            // 0: ADD{S} Rd,Rs,Rn   ;add register Rd=Rs+Rn
            op2 = cpu->getRegister((instruction & 0x01C0) >> 6);
            result = rsVal + op2;
            carryFlag = aluAddSetsCarryBit(rsVal, op2);
            overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 1: {
            // 1: SUB{S} Rd,Rs,Rn   ;subtract register   Rd=Rs-Rn
            op2 = cpu->getRegister((instruction & 0x01C0) >> 6);
            result = rsVal - op2;
            carryFlag = aluSubtractSetsCarryBit(rsVal, op2);
            overflowFlag = aluSubtractSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 2: {
            // 2: ADD{S} Rd,Rs,#nn  ;add immediate       Rd=Rs+nn
            op2 = (instruction & 0x01C0) >> 6;
            result = rsVal + op2;
            carryFlag = aluAddSetsCarryBit(rsVal, op2);
            overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 3: {
            // 3: SUB{S} Rd,Rs,#nn  ;subtract immediate  Rd=Rs-nn
            op2 = (instruction & 0x01C0) >> 6;
            result = rsVal - op2;
            carryFlag = aluSubtractSetsCarryBit(rsVal, op2);
            overflowFlag = aluSubtractSetsOverflowBit(rsVal, op2, result);
            break;
        }      
    }
    signFlag = aluSetsSignBit(result);
    zeroFlag = aluSetsZeroBit(result);

    cpu->setRegister(rd, result);
    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.N = signFlag;
    cpu->cpsr.V = overflowFlag;

    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::immHandler(uint16_t instruction,
                                                           ARM7TDMI *cpu) {
    assert((instruction & 0xE000) == 0x2000);
    uint8_t opcode = (instruction & 0x1800) >> 11;

    uint32_t offset = instruction & 0x00FF;
    uint8_t rd = (instruction & 0x0700) >> 8;

    bool carryFlag;
    bool signFlag;
    bool overflowFlag;
    bool zeroFlag;

    switch (opcode) {
        case 0: {
            // 00b: MOV{S} Rd,#nn      ;move     Rd   = #nn
            cpu->setRegister(rd, offset);
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            break;
        }
        case 1: {
            // 01b: CMP{S} Rd,#nn      ;compare  Void = Rd - #nn
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result = rdVal - offset;
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
            break;
        }
        case 2: {
            // 10b: ADD{S} Rd,#nn      ;add      Rd   = Rd + #nn
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result = rdVal + offset;
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = aluAddSetsCarryBit(rdVal, offset);
            overflowFlag = aluAddSetsOverflowBit(rdVal, offset, result);
            cpu->setRegister(rd, result);
            break;
        }
        case 3: {
            // 11b: SUB{S} Rd,#nn      ;subtract Rd   = Rd - #nn
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result = rdVal - offset;
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
            cpu->setRegister(rd, result);
            break;
        }
    }

    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.N = signFlag;
    cpu->cpsr.V = overflowFlag;

    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::aluHandler(uint16_t instruction,
                                                           ARM7TDMI *cpu) {

    assert((instruction & 0xFC00) == 0x4000);

    uint8_t opcode = (instruction & 0x03C0) >> 6;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    uint32_t result;
    bool carryFlag, overflowFlag, signFlag, zeroFlag;

    switch(opcode) {
        case 0: {
            // 0: AND{S} Rd,Rs     ;AND logical       Rd = Rd AND Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            result = rdVal & rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 1: {
            //  1: EOR{S} Rd,Rs     ;XOR logical       Rd = Rd XOR Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            result = rdVal ^ rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break; 
        }
        case 2: {
            // 2: LSL{S} Rd,Rs     ;log. shift left   Rd = Rd << (Rs AND 0FFh)
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            result = aluShiftLsl(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO: 
            carryFlag = !(offset) ? cpu->cpsr.C : (rdVal >> (32 - offset)) & 1;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 3: {
            // 3: LSR{S} Rd,Rs     ;log. shift right  Rd = Rd >> (Rs AND 0FFh)
            break;
        }
        case 4: {
            // 4: ASR{S} Rd,Rs     ;arit shift right  Rd = Rd SAR (Rs AND 0FFh)
            break;
        }
        case 5: {
            // 5: ADC{S} Rd,Rs     ;add with carry    Rd = Rd + Rs + Cy
            break;
        }
        case 6: {
            // 6: SBC{S} Rd,Rs     ;sub with carry    Rd = Rd - Rs - NOT Cy
            break;
        }
        case 7: {
            // 7: ROR{S} Rd,Rs     ;rotate right      Rd = Rd ROR (Rs AND 0FFh)
            break;
        }
        case 8: {
            // 8: TST    Rd,Rs     ;test            Void = Rd AND Rs
            break;
        }
        case 9: {
            // 9: NEG{S} Rd,Rs     ;negate            Rd = 0 - Rs
            break;
        }
        case 0xA: {
            // A: CMP    Rd,Rs     ;compare         Void = Rd - Rs
            break;
        }
        case 0xB: {
            // B: CMN    Rd,Rs     ;neg.compare     Void = Rd + Rs
            break;
        }
        case 0xC: {
            // C: ORR{S} Rd,Rs     ;OR logical        Rd = Rd OR Rs
            break;
        }
        case 0xD: {
            // D: MUL{S} Rd,Rs     ;multiply          Rd = Rd * Rs
            break;
        }
        case 0xE: {
            // E: BIC{S} Rd,Rs     ;bit clear         Rd = Rd AND NOT Rs
            break; 
        }
        case 0xF: {
            // F: MVN{S} Rd,Rs     ;not               Rd = NOT Rs
            break;
        }
    }

    return {};
}