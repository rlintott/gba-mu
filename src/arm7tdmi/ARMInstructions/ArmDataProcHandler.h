
#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"


template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armDataProcHandler(uint32_t instruction, ARM7TDMI* cpu) {
    // shift op2
    constexpr bool i = (op & 0x200);
    constexpr bool r = (op & 0x1);
    constexpr uint8_t opcode = ((op & 0x1E0) >> 5);
    constexpr bool s = (op & 0x010);

    uint32_t op2; 
    bool shiftCarryBit;

    // -----SHIFTING------
    if constexpr(i) {
        /*
            The immediate operand rotate field is a 4 bit unsigned integer
            which specifies a shift operation on the 8 bit immediate value.
            This value is zero extended to 32 bits, and then subject to a
            rotate right by twice the value in the rotate field.
        */

        uint32_t imm = instruction & 0x000000FF;
        uint8_t is = (instruction & 0x00000F00) >> 7U;
        op2 = ARM7TDMI::aluShiftRor(imm, is % 32);

        // carry out bit is the least significant discarded bit of rm
        if (is > 0) {
            shiftCarryBit = (imm >> (is - 1)) & 0x1;
        } else {
            shiftCarryBit = cpu->cpsr.C;
        }
    } else {
        /* ~~~~~~~~~ else: shifted register value as 2nd operand ~~~~~~~~~~ */
        uint8_t shiftType = (instruction & 0x00000060) >> 5U;
        uint8_t rmIndex = instruction & 0x0000000F;
        uint32_t rm = cpu->getRegister(rmIndex);
        // see comment in opcode functions for explanation why we're doing this

        if constexpr(!i && r) {
            if(rmIndex == ARM7TDMI::PC_REGISTER) {
                rm += 8;
            }        
        } else {
            if(rmIndex == ARM7TDMI::PC_REGISTER) {
                rm += 4;
            }
        }

        uint32_t shiftAmount;
        bool doShift = true;

        if constexpr(r) {  // register as shift amount
            uint8_t rsIndex = (instruction & 0x00000F00) >> 8U;
            assert(rsIndex != 15);
            shiftAmount = cpu->getRegister(rsIndex) & 0x000000FF;
            if(shiftAmount == 0) {
                op2 = rm;
                shiftCarryBit = cpu->cpsr.C;
                doShift = false;
            }
        } else {  // immediate as shift amount
            shiftAmount = (instruction & 0x00000F80) >> 7U;
        }

        bool immOpIsZero = r ? false : (shiftAmount == 0);
        if(doShift) {
            if (shiftType == 0) {  // Logical Shift Left
                /*
                    A logical shift left (LSL) takes the contents of
                    Rm and moves each bit by the specified amount
                    to a more significant position. The least significant
                    bits of the result are filled with zeros, and the high bits
                    of Rm which do not map into the result are discarded, except
                    that the least significant discarded bit becomes the shifter
                    carry output which may be latched into the C bit of the CPSR
                    when the ALU operation is in the logical class
                */
                if (!immOpIsZero) {
                    op2 = ARM7TDMI::aluShiftLsl(rm, shiftAmount);
                    shiftCarryBit = (shiftAmount > 32) ? 0 : ((rm >> (32 - shiftAmount)) & 1);
                } else {  // no op performed, carry flag stays the same
                    op2 = rm;
                    shiftCarryBit = cpu->cpsr.C;
                }
            } else if (shiftType == 1) {  // Logical Shift Right
                /*
                    A logical shift right (LSR) is similar, but the contents
                    of Rm are moved to less significant positions in the result
                */
                if (!immOpIsZero) {
                    op2 = ARM7TDMI::aluShiftLsr(rm, shiftAmount);
                    shiftCarryBit = (shiftAmount > 32) ? 0 : ((rm >> (shiftAmount - 1)) & 1);
                } else {
                    /*
                        The form of the shift field which might be expected to
                        correspond to LSR #0 is used to encode LSR #32, which has a
                        zero result with bit 31 of Rm as the carry output
                    */
                    op2 = 0;
                    shiftCarryBit = rm >> 31;
                }
            } else if (shiftType == 2) {  // Arithmetic Shift Right
                /*
                    An arithmetic shift right (ASR) is similar to logical shift right,
                    except that the high bits are filled with bit 31 of Rm instead of
                zeros. This preserves the sign in 2's complement notation
                */
                if (!immOpIsZero) {
                    op2 = ARM7TDMI::aluShiftAsr(rm, shiftAmount);
                    shiftCarryBit = (shiftAmount >= 32) ? (rm & 0x80000000) : ((rm >> (shiftAmount - 1)) & 1);
                } else {
                    /*
                        The form of the shift field which might be expected to give ASR
                    #0 is used to encode ASR #32. Bit 31 of Rm is again used as the
                    carry output, and each bit of operand 2 is also equal to bit 31
                    of Rm.
                    */
                    op2 = ARM7TDMI::aluShiftAsr(rm, 32);
                    shiftCarryBit = rm >> 31;
                }
            } else {  // Rotating Shift
                /*
                    Rotate right (ROR) operations reuse the bits which “overshoot”
                    in a logical shift right operation by reintroducing them at the
                    high end of the result, in place of the zeros used to fill the high
                    end in logical right operation
                */
                if (!immOpIsZero) {
                    op2 = ARM7TDMI::aluShiftRor(rm, shiftAmount % 32);
                    shiftCarryBit = (rm >> ((shiftAmount % 32) - 1)) & 1;
                } else {
                    /*
                        The form of the shift field which might be expected to give ROR
                    #0 is used to encode a special function of the barrel shifter,
                        rotate right extended (RRX). This is a rotate right by one bit
                    position of the 33 bit quantity formed by appending the CPSR C
                    flag to the most significant end of the contents of Rm as shown
                    */
                    op2 = rm >> 1;
                    op2 = op2 | (((uint32_t)(cpu->cpsr.C)) << 31);
                    shiftCarryBit = rm & 1;
                }
            }
        }
    }

    uint8_t rd = getRd(instruction);
    uint8_t rn = getRn(instruction);
    //uint8_t opcode = getOpcode(instruction);
    bool carryBit = (cpu->cpsr).C;
    bool overflowBit = (cpu->cpsr).V;
    bool signBit = (cpu->cpsr).N;
    bool zeroBit = (cpu->cpsr).Z;

    uint32_t rnVal;
    // if rn == pc regiser, have to add to it to account for pipelining /
    // prefetching
    // TODO probably dont need this logic if pipelining is emulated
    if constexpr(!i && r) {
        if (rn != ARM7TDMI::PC_REGISTER) {
            rnVal = cpu->getRegister(rn);
        } else {
            rnVal = cpu->getRegister(rn) + 8;
        }
    } else {
        if (rn != ARM7TDMI::PC_REGISTER) {
            rnVal = cpu->getRegister(rn);
        } else {
            rnVal = cpu->getRegister(rn) + 4;
        }
    }

    if constexpr(opcode == 0x0) {  // AND
        uint32_t result = rnVal & op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = shiftCarryBit;
    } else if constexpr(opcode == 0x1) {  // EOR
        uint32_t result = rnVal ^ op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = shiftCarryBit;
    } else if constexpr(opcode == 0x2) {  // SUB
        uint32_t result = rnVal - op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = ARM7TDMI::aluSubtractSetsCarryBit(rnVal, op2);
        overflowBit = ARM7TDMI::aluSubtractSetsOverflowBit(rnVal, op2, result);
    } else if constexpr(opcode == 0x3) {  // RSB
        uint32_t result = op2 - rnVal;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = ARM7TDMI::aluSubtractSetsCarryBit(op2, rnVal);
        overflowBit = ARM7TDMI::aluSubtractSetsOverflowBit(op2, rnVal, result);
    } else if constexpr(opcode == 0x4) {  // ADD
        uint32_t result = rnVal + op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = ARM7TDMI::aluAddSetsCarryBit(rnVal, op2);
        overflowBit = ARM7TDMI::aluAddSetsOverflowBit(rnVal, op2, result);
    } else if constexpr(opcode == 0x5) {  // ADC
        uint64_t result = (uint64_t)rnVal + (uint64_t)op2 + (uint64_t)(cpu->cpsr).C;
        cpu->setRegister(rd, (uint32_t)result);
        zeroBit = ARM7TDMI::aluSetsZeroBit((uint32_t)result);
        signBit = ARM7TDMI::aluSetsSignBit((uint32_t)result);
        carryBit = ARM7TDMI::aluAddWithCarrySetsCarryBit(result);
        overflowBit = ARM7TDMI::aluAddWithCarrySetsOverflowBit(rnVal, op2, (uint32_t)result, cpu);
    } else if constexpr(opcode == 0x6) {  // SBC
        uint64_t result = (uint64_t)rnVal + ~((uint64_t)op2) + (uint64_t)(cpu->cpsr.C);
        cpu->setRegister(rd, (uint32_t)result);
        zeroBit = ARM7TDMI::aluSetsZeroBit((uint32_t)result);
        signBit = ARM7TDMI::aluSetsSignBit((uint32_t)result);
        carryBit = ARM7TDMI::aluSubWithCarrySetsCarryBit(result);
        overflowBit = ARM7TDMI::aluSubWithCarrySetsOverflowBit(rnVal, op2, (uint32_t)result, cpu);
    } else if constexpr(opcode == 0x7) {  // RSC
        uint64_t result = (uint64_t)op2 + ~((uint64_t)rnVal) + (uint64_t)(cpu->cpsr.C);
        cpu->setRegister(rd, (uint32_t)result);
        zeroBit = ARM7TDMI::aluSetsZeroBit((uint32_t)result);
        signBit = ARM7TDMI::aluSetsSignBit((uint32_t)result);
        carryBit = ARM7TDMI::aluSubWithCarrySetsCarryBit(result);
        overflowBit = ARM7TDMI::aluSubWithCarrySetsOverflowBit(op2, rnVal, (uint32_t)result, cpu);
    } else if constexpr(opcode == 0x8) {  // TST
        uint32_t result = rnVal & op2;
        carryBit = shiftCarryBit;
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
    } else if constexpr(opcode == 0x9) {  // TEQ
        uint32_t result = rnVal ^ op2;
        carryBit = shiftCarryBit;
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
    } else if constexpr(opcode == 0xA) {  // CMP
        uint32_t result = rnVal - op2;
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = ARM7TDMI::aluSubtractSetsCarryBit(rnVal, op2);
        overflowBit = ARM7TDMI::aluSubtractSetsOverflowBit(rnVal, op2, result);
    } else if constexpr(opcode == 0xB) {  // CMN
        uint32_t result = rnVal + op2;
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = ARM7TDMI::aluAddSetsCarryBit(rnVal, op2);
        overflowBit = ARM7TDMI::aluAddSetsOverflowBit(rnVal, op2, result);
    } else if constexpr(opcode == 0xC) {  // ORR
        uint32_t result = rnVal | op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = shiftCarryBit;
    } else if constexpr(opcode == 0xD) {  // MOV
        uint32_t result = op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        carryBit = shiftCarryBit;
        signBit = ARM7TDMI::aluSetsSignBit(result);
    } else if constexpr(opcode == 0xE) {  // BIC
        uint32_t result = rnVal & (~op2);
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = shiftCarryBit;
    } else {  // MVN
        uint32_t result = ~op2;
        cpu->setRegister(rd, result);
        zeroBit = ARM7TDMI::aluSetsZeroBit(result);
        signBit = ARM7TDMI::aluSetsSignBit(result);
        carryBit = shiftCarryBit;
    }

    if constexpr(s) {
        if(rd != ARM7TDMI::PC_REGISTER) {
            cpu->cpsr.Z = zeroBit;
            cpu->cpsr.C = carryBit;
            cpu->cpsr.N = signBit;
            cpu->cpsr.V = overflowBit;
        } else {
            cpu->cpsr = *(cpu->getCurrentModeSpsr());
            cpu->switchToMode(ARM7TDMI::Mode((*(cpu->getCurrentModeSpsr())).Mode));
        }
    }

    ARM7TDMI::Cycles cycles = {.nonSequentialCycles = 0,
                     .sequentialCycles = 1,
                     .internalCycles = 0,
                     .waitState = 0};
    
    // TODO: can potentially optimize a bit by using already existing condition checks earlier in code

    if constexpr (!i && r) {
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
        if(rd == ARM7TDMI::PC_REGISTER) {
            return ARM7TDMI::FetchPCMemoryAccess::BRANCH;
        } else {
            return ARM7TDMI::FetchPCMemoryAccess::NONSEQUENTIAL;
        }
    }
    if(rd == ARM7TDMI::PC_REGISTER) {
        return ARM7TDMI::FetchPCMemoryAccess::BRANCH;;
    } else {
        return ARM7TDMI::FetchPCMemoryAccess::SEQUENTIAL;
    }

}