#include "ARM7TDMI.h"
#include "Bus.h"
#include <bit>
#include <bitset>


ARM7TDMI::ARM7TDMI() {
    dataProccessingOpcodes = {  
        {"AND", &ARM7TDMI::AND}, 
    };

    undefinedOpcode = {"UNDEFINED", &ARM7TDMI::UNDEF};
}


ARM7TDMI::~ARM7TDMI() {
}


void ARM7TDMI::executeInstructionCycle() {
    // read from program counter
    // uint32_t rawInstruction = bus->read(registers[PC_REGISTER]);
    uint32_t rawInstruction = 0;

    if(cpsr.T) { // check state bit, is CPU in ARM state?
        Instruction instruction = decodeInstruction(rawInstruction);

    } else { // THUMB state

    }
}


ARM7TDMI::Cycles ARM7TDMI::AND(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    /* ~~~~~~~~~~~ the operation ~~~~~~~~~~~~` */
    uint32_t result = getRegister(rn) & shiftResult.op2;
    setRegister(rd, result);
    
    /* ~~~~~~~~~~~ updating CPSR flags ~~~~~~~~~~~~` */
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { // flags not affected
        
    }

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::UNDEF(uint32_t instruction) {
    return {};
}


/**
ARM Binary Opcode Format
  |..3 ..................2 ..................1 ..................0|
  |1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
  |_Cond__|0_0_0|__Op__|S|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| DataProc
  |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Rs___|0|Typ|1|__Rm___| DataProc
  |_Cond__|0_0_1|___Op__|S|__Rn___|__Rd___|_Shift_|___Immediate___| DataProc
  |_Cond__|0_0_1_1_0_0_1_0_0_0_0_0_1_1_1_1_0_0_0_0|_____Hint______| ARM11:Hint
  |_Cond__|0_0_1_1_0|P|1|0|_Field_|__Rd___|_Shift_|___Immediate___| PSR Imm
  |_Cond__|0_0_0_1_0|P|L|0|_Field_|__Rd___|0_0_0_0|0_0_0_0|__Rm___| PSR Reg
  |_Cond__|0_0_0_1_0_0_1_0_1_1_1_1_1_1_1_1_1_1_1_1|0_0|L|1|__Rn___| BX,BLX
  |1_1_1_0|0_0_0_1_0_0_1_0|_____immediate_________|0_1_1_1|_immed_| ARM9:BKPT
  |_Cond__|0_0_0_1_0_1_1_0_1_1_1_1|__Rd___|1_1_1_1|0_0_0_1|__Rm___| ARM9:CLZ
  |_Cond__|0_0_0_1_0|Op_|0|__Rn___|__Rd___|0_0_0_0|0_1_0_1|__Rm___| ARM9:QALU
  |_Cond__|0_0_0_0_0_0|A|S|__Rd___|__Rn___|__Rs___|1_0_0_1|__Rm___| Multiply
  |_Cond__|0_0_0_0_0_1_0_0|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| ARM11:UMAAL
  |_Cond__|0_0_0_0_1|U|A|S|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| MulLong
  |_Cond__|0_0_0_1_0|Op_|0|Rd/RdHi|Rn/RdLo|__Rs___|1|y|x|0|__Rm___| MulHalfARM9
  |_Cond__|0_0_0_1_0|B|0_0|__Rn___|__Rd___|0_0_0_0|1_0_0_1|__Rm___| TransSwp12
  |_Cond__|0_0_0_1_1|_Op__|__Rn___|__Rd___|1_1_1_1|1_0_0_1|__Rm___| ARM11:LDREX
  |_Cond__|0_0_0|P|U|0|W|L|__Rn___|__Rd___|0_0_0_0|1|S|H|1|__Rm___| TransReg10
  |_Cond__|0_0_0|P|U|1|W|L|__Rn___|__Rd___|OffsetH|1|S|H|1|OffsetL| TransImm10
  |_Cond__|0_1_0|P|U|B|W|L|__Rn___|__Rd___|_________Offset________| TransImm9
  |_Cond__|0_1_1|P|U|B|W|L|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| TransReg9
  |_Cond__|0_1_1|________________xxx____________________|1|__xxx__| Undefined
  |_Cond__|0_1_1|Op_|x_x_x_x_x_x_x_x_x_x_x_x_x_x_x_x_x_x|1|x_x_x_x| ARM11:Media
  |1_1_1_1_0_1_0_1_0_1_1_1_1_1_1_1_1_1_1_1_0_0_0_0_0_0_0_1_1_1_1_1| ARM11:CLREX
  |_Cond__|1_0_0|P|U|S|W|L|__Rn___|__________Register_List________| BlockTrans
  |_Cond__|1_0_1|L|___________________Offset______________________| B,BL,BLX
  |_Cond__|1_1_0|P|U|N|W|L|__Rn___|__CRd__|__CP#__|____Offset_____| CoDataTrans
  |_Cond__|1_1_0_0_0_1_0|L|__Rn___|__Rd___|__CP#__|_CPopc_|__CRm__| CoRR ARM9
  |_Cond__|1_1_1_0|_CPopc_|__CRn__|__CRd__|__CP#__|_CP__|0|__CRm__| CoDataOp
  |_Cond__|1_1_1_0|CPopc|L|__CRn__|__Rd___|__CP#__|_CP__|1|__CRm__| CoRegTrans
  |_Cond__|1_1_1_1|_____________Ignored_by_Processor______________| SWI
*/  
ARM7TDMI::AsmOpcodeType ARM7TDMI::getAsmOpcodeType(uint32_t instruction) {
    // IMPORTANT: parse from highest to lowest specificity
}


ARM7TDMI::Instruction ARM7TDMI::decodeInstruction(uint32_t rawInstruction) {
}

// Comment documentation sourced from the ARM7TDMI Data Sheet. 
ARM7TDMI::AluShiftResult ARM7TDMI::aluShift(uint32_t instruction, bool i, bool r) {
    if(i) { // shifting immediate value as 2nd operand
        /*
            The immediate operand rotate field is a 4 bit unsigned integer 
            which specifies a shift operation on the 8 bit immediate value. 
            This value is zero extended to 32 bits, and then subject to a 
            rotate right by twice the value in the rotate field.
        */
        uint32_t rm = instruction & 0x000000FF;
        uint8_t is = (instruction & 0x00000F00) >> 7U;
        uint32_t op2 = aluShiftRor(rm, is);
        uint8_t carry;
        // carry out bit is the least significant discarded bit of rm
        if(is > 0) {
            carry = (rm >> (is - 1)); 
        }
        return {op2, carry};
    }

    /* ~~~~~~~~~ else: shifting register value as 2nd operand ~~~~~~~~~~ */
    uint8_t shiftType = (instruction & 0x00000060) >> 5U;
    uint32_t op2;
    uint32_t rm = r ? getRegister(instruction & 0x0000000F) : getRegister(instruction & 0x0000000F);
    uint32_t shiftAmount;
    uint8_t carry;
    bool immOpIsZero = r ? false : shiftAmount == 0;

    if(r) { // register as second operand
        uint8_t rs = (instruction & 0x00000F00) >> 8U;
        shiftAmount = getRegister(rs) & 0x000000FF;
    } else { // immediate as second operand
        shiftAmount = instruction & 0x00000F80 >> 7U;
    }

    if(shiftType == 0) {  // Logical Shift Left
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
        if(!immOpIsZero) {
            op2 = aluShiftLsl(rm, shiftAmount);
            carry = (rm >> (32 - shiftAmount)) & 1;
        } else { // no op performed, carry flag stays the same
            op2 = rm;
            carry = cpsr.C;
        }
    }
    else if(shiftType == 1) { // Logical Shift Right
        /*
            A logical shift right (LSR) is similar, but the contents 
            of Rm are moved to less significant positions in the result    
        */
        if(!immOpIsZero) {
            op2 = aluShiftLsr(rm, shiftAmount);
            carry = (rm >> (shiftAmount - 1)) & 1;
        } else {
            /*
                The form of the shift field which might be expected to 
                correspond to LSR #0 is used to encode LSR #32, which has a 
                zero result with bit 31 of Rm as the carry output
            */
            op2 = 0;
            carry = rm >> 31;
        }
    }
    else if(shiftType == 2) { // Arithmetic Shift Right
        /*
            An arithmetic shift right (ASR) is similar to logical shift right, 
            except that the high bits are filled with bit 31 of Rm instead of zeros. 
            This preserves the sign in 2's complement notation
        */
        if(!immOpIsZero) {
            op2 = aluShiftAsr(rm, shiftAmount);
            carry = rm >> 31;
        } else {
            /*
                The form of the shift field which might be expected to give ASR #0 
                is used to encode ASR #32. Bit 31 of Rm is again used as the carry output, 
                and each bit of operand 2 is also equal to bit 31 of Rm. 
            */
            op2 = aluShiftAsr(rm, 32);
            carry = rm >> 31;
        }
    }
    else { // Rotating Shift
        /*
            Rotate right (ROR) operations reuse the bits which “overshoot” 
            in a logical shift right operation by reintroducing them at the
            high end of the result, in place of the zeros used to fill the high 
            end in logical right operation
        */
        if(!immOpIsZero) {
            op2 = aluShiftRor(rm, shiftAmount);
            carry = (rm >> (shiftAmount - 1)) & 1;
        } else {
            /*
                The form of the shift field which might be expected to give ROR #0 
                is used to encode a special function of the barrel shifter, 
                rotate right extended (RRX). This is a rotate right by one bit position 
                of the 33 bit quantity formed by appending the CPSR C flag to the most 
                significant end of the contents of Rm as shown
            */
            op2 = rm >> 1;
            op2 = op2 | (((uint32_t)cpsr.C) << 31);
            carry = rm & 1;
        }
    }

    return {op2, carry};
}


uint32_t ARM7TDMI::aluShiftLsl(uint32_t value, uint8_t shift) {
    return value << shift;
}


uint32_t ARM7TDMI::aluShiftLsr(uint32_t value, uint8_t shift) {
    return value >> shift;
}


uint32_t ARM7TDMI::aluShiftAsr(uint32_t value, uint8_t shift) {
    // assuming that the compiler implements arithmetic right shift on signed ints 
    // TODO: make more portable
    return (uint32_t)(((int32_t)value) >> shift);
}


uint32_t ARM7TDMI::aluShiftRor(uint32_t value, uint8_t shift) {
    assert (shift < 32U);
    return (value >> shift) | (value << (-shift & 31U));
}


uint32_t ARM7TDMI::aluShiftRrx(uint32_t value, uint8_t shift) {
    assert (shift < 32U);
    uint32_t rrxMask = cpsr.C;
    return ((value >> shift) | (value << (-shift & 31U))) | (rrxMask << 31U);
}


void ARM7TDMI::aluUpdateCpsrFlags(AluOperationType opType, uint32_t result, uint32_t op2, uint8_t rd) {
    if(rd != PC_REGISTER) {
        if(opType == LOGICAL) {
            // dont need to update C flag, already updated it in ARM7TDMI::aluShift
            cpsr.Z = (result == 0);
            cpsr.N = (result >> 31);
        } else if(opType == ARITHMETIC) {
            cpsr.Z = (result == 0);
            cpsr.N = (result >> 31);
        } else { // opType == COMPARISON
            

        }
    }

}


ARM7TDMI::AluOperationType ARM7TDMI::getAluOperationType(uint32_t instruction) {
    uint8_t opcode = (instruction & 0x01E00000) >> 21;
    return aluOperationTypeTable[opcode];
}


ARM7TDMI::ProgramStatusRegister ARM7TDMI::getModeSpsr() {
    if(cpsr.Mode == USER) {
        return cpsr;
    } else if(cpsr.Mode == FIQ) {
        return SPSR_fiq;
    } else if(cpsr.Mode == IRQ) {
        return SPSR_irq;
    } else if(cpsr.Mode == SUPERVISOR) {
        return SPSR_svc;
    } else if(cpsr.Mode == ABORT) {
        return SPSR_abt;
    } else if(cpsr.Mode == UNDEFINED) {
        return SPSR_und;
    } else {
        return cpsr;
    }
}


uint32_t ARM7TDMI::getRegister(uint8_t index) {
    if(0 <= index && index < 8) {
        return registers[index];
    } 

    if(cpsr.Mode == USER) {
        return registers[index];
    } else if(cpsr.Mode == FIQ) {
        if(7 < index && index < 15) {
            return fiqRegisters[index];
        } else {
            return registers[index];
        }
    } else if(cpsr.Mode == IRQ) {
        if(12 < index && index < 15) {
            return irqRegisters[index];
        } else {
            return registers[index];
        }
    } else if(cpsr.Mode == SUPERVISOR) {
        if(12 < index && index < 15) {
            return svcRegisters[index];
        } else {
            return registers[index];
        }       
    } else if(cpsr.Mode == ABORT) {
        if(12 < index && index < 15) {
            return abtRegisters[index];
        } else {
            return registers[index];
        }        
    } else if(cpsr.Mode == UNDEFINED) {
        if(12 < index && index < 15) {
            return undRegisters[index];
        } else {
            return registers[index];
        }        
    } else {
        return 0;
    }
}


void ARM7TDMI::setRegister(uint8_t index, uint32_t value) {
    if(0 <= index && index < 8) {
        registers[index] = value;
    } 

    if(cpsr.Mode == USER) {
        registers[index] = value;
    } else if(cpsr.Mode == FIQ) {
        if(7 < index && index < 15) {
            fiqRegisters[index] = value;
        } else {
            registers[index] = value;
        }
    } else if(cpsr.Mode == IRQ) {
        if(12 < index && index < 15) {
            irqRegisters[index] = value;
        } else {
            registers[index] = value;
        }
    } else if(cpsr.Mode == SUPERVISOR) {
        if(12 < index && index < 15) {
            svcRegisters[index] = value;
        } else {
            registers[index] = value;
        }       
    } else if(cpsr.Mode == ABORT) {
        if(12 < index && index < 15) {
            abtRegisters[index] = value;
        } else {
            registers[index] = value;
        }        
    } else if(cpsr.Mode == UNDEFINED) {
        if(12 < index && index < 15) {
            undRegisters[index] = value;
        } else {
            registers[index] = value;
        }        
    } else {
    }
}
