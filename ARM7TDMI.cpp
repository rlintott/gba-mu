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
    // uint32_t rawInstruction = bus->read(registers[15]);
    uint32_t rawInstruction = 0;

    if(cpsr.T) { // check state bit, is CPU in ARM state?
        Instruction instruction = decodeInstruction(rawInstruction);

    } else { // THUMB state

    }
}


ARM7TDMI::Cycles ARM7TDMI::AND(uint32_t instruction) {
    uint32_t op2 = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    uint32_t result = getRegister(rn) & op2;
    setRegister(rd, result);
    
    AluOperationType opType = getAluOperationType(instruction);

    // updateCpsrFlags(opType, result, op2);
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


uint32_t ARM7TDMI::aluShift(uint32_t instruction, bool i, bool r) {
    if(i) { // immediate shift

        uint8_t op2 = instruction & 0x000000FF;
        uint8_t is = (instruction & 0x00000F00) >> 8U;
        op2 = aluShiftRor(op2, is);
        cpsr.C = (op2 >> 7U); 
        return op2;
    }

    // else: register shift

    uint8_t shiftType = (instruction & 0x00000060) >> 5U;
    uint32_t op2;
    uint32_t shiftAmount;
    bool special = false;
    bool updateCpsr = true;

    if(r) { // register as second operand

        op2 = getRegister(instruction & 0x0000000F);
        uint8_t rs = (instruction & 0x00000F00) >> 8U;
        shiftAmount = getRegister(rs) & 0x000000FF;
    } else { // immediate as second operand

        op2 = getRegister(instruction & 0x0000000F);
        shiftAmount = instruction & 0x00000F80 >> 7U;
        special = shiftAmount == 0;
    }

    if(shiftType == 0) {  // Logical Shift Left
        if(!special) {
            op2 = aluShiftLsl(op2, shiftAmount);
        } else { // no op performed, no need to chage carry flag
            updateCpsr = false;
        }
    }
    else if(shiftType == 1) { // Logical Shift Right
        if(!special) {
            op2 = aluShiftLsr(op2, shiftAmount);
        } else {
            op2 = aluShiftLsr(op2, 32);
        }
    }
    else if(shiftType == 2) { // Arithmetic Shift Right
        if(!special) {
            op2 = aluShiftAsr(op2, shiftAmount);
        } else {
            op2 = aluShiftAsr(op2, 32);
        }
    }
    else { // Rotating Shift
        if(!special) {
            op2 = aluShiftRor(op2, shiftAmount);
        } else {
            op2 = aluShiftRrx(op2, 1U);
            updateCpsr = false;
            // no need to change carry flag
        }
    }
    if((instruction & 0x00100000) && updateCpsr) {
        cpsr.C = (op2 & 0x80000000) >> 31U;
    }

    return op2;
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
    assert (shift<32U);
    return (value>>shift) | (value<<(-shift&31U));
}


uint32_t ARM7TDMI::aluShiftRrx(uint32_t value, uint8_t shift) {
    assert (shift<32U);
    uint32_t rrxMask = cpsr.C;
    return ((value>>shift) | (value<<(-shift&31U))) | (rrxMask << 31U);
}


void ARM7TDMI::aluUpdateCpsrFlags(AluOperationType opType, uint32_t result, uint32_t op2) {
}


ARM7TDMI::AluOperationType ARM7TDMI::getAluOperationType(uint32_t instruction) {
    uint8_t opcode = (instruction & 0x01E00000) >> 21;
    return aluOperationTypeTable[opcode];
}