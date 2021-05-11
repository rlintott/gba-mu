#include "ARM7TDMI.h"
#include "Bus.h"
#include <bit>
#include <bitset>
#include <iostream>


ARM7TDMI::ARM7TDMI() {
    cpsr.Mode = SUPERVISOR;
    cpsr.T = 0; // set to ARM state
    setRegister(PC_REGISTER, BOOT_LOCATION);
}


ARM7TDMI::~ARM7TDMI() {
}


void ARM7TDMI::step() {
    // read from program counter
    uint32_t rawInstruction = bus->read(registers[PC_REGISTER]);

    if(cpsr.T) { // check state bit, is CPU in ARM state?
        Cycles cycles = executeInstruction(rawInstruction);

    } else { // THUMB state

    }
}


void ARM7TDMI::connectBus(Bus* bus) {
    this->bus = bus;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALU OPERATIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// TODO: put assertions for every unexpected or unallowed circumstance (for deubgging)
// TODO: cycle calculation
// TODO: REFACTOR, there is lots of duplicated code, will be horrible to debug

ARM7TDMI::Cycles ARM7TDMI::SUB(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    /*
        When using R15 as operand (Rm or Rn), the returned value depends 
        on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    */
    uint32_t result;
    uint32_t op2;
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
        op2 = shiftResult.op2;
        result = rnValue - op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
        op2 = shiftResult.op2;
        result = rnValue - op2;
    } else {
        rnValue = getRegister(rn) + 8;
        op2 = shiftResult.op2;
        result = rnValue - op2;
    }

    setRegister(rd, result);
    
    /* ~~~~~~~~~~~ updating CPSR flags ~~~~~~~~~~~~` */
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        // For a subtraction, including the comparison instruction CMP,
        // C is set to 0 if the subtraction produced a borrow 
        // (that is, an unsigned underflow), and to 1 otherwise.
        // source: https://developer.arm.com/documentation/dui0801/a/Condition-Codes/Carry-flag
        cpsr.C = !(rnValue < op2);
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // *OVERFLOW* 0 1 1 (subtracting a negative is the same as adding a positive)
        // *OVERFLOW* 1 0 0 (subtracting a positive is the same as adding a negative)
        cpsr.V = (!(rnValue & 0x80000000) && (op2 & 0x80000000) && (result & 0x80000000)) || 
                 ((rnValue & 0x80000000) && !(op2 & 0x80000000) && !(result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { // flags not affected
        
    }

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::ADD(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    /*
        When using R15 as operand (Rm or Rn), the returned value depends 
        on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    */
    uint32_t result;
    uint32_t op2;
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
        op2 = shiftResult.op2;
        result = rnValue + op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
        op2 = shiftResult.op2;
        result = rnValue + op2;
    } else {
        rnValue = getRegister(rn) + 8;
        op2 = shiftResult.op2;
        result = rnValue + op2;
    }

    setRegister(rd, result);
    
    /* ~~~~~~~~~~~ updating CPSR flags ~~~~~~~~~~~~` */
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        // For an addition, including the comparison instruction CMN, 
        // C is set to 1 if the addition produced a carry (that is, an unsigned overflow),
        // and to 0 otherwise. source: https://developer.arm.com/documentation/dui0801/a/Condition-Codes/Carry-flag
        //cpsr.C = ((int32_t)(0xFFFFFFFF + op2)) < 0); 
        cpsr.C = (0xFFFFFFFF - op2) < rnValue;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // *OVERFLOW* 1 1 -> 0
        // *OVERFLOW* 0 0 -> 1
        cpsr.V = ((rnValue & 0x80000000) && (op2 & 0x80000000) && !(result & 0x80000000)) || 
                 (!(rnValue & 0x80000000) && !(op2 & 0x80000000) && (result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { // flags not affected
        
    }

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::ADC(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint64_t result; // temporary 64 bit result for ease of calculating carry bit
    uint32_t op2;
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
        op2 = shiftResult.op2;
        result = rnValue + op2 + cpsr.C;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
        op2 = shiftResult.op2;
        result = rnValue + op2 + cpsr.C;
    } else {
        rnValue = getRegister(rn) + 8;
        op2 = shiftResult.op2;
        result = rnValue + op2 + cpsr.C;
    }

    setRegister(rd, (uint32_t)result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        // if unsigned int overflow, then result carried
        cpsr.C = (result >> 32);
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // if a + b overflowed OR (a + b) + c overflowed, then result overflowed
        cpsr.V = ((rnValue & 0x80000000) && (op2 & 0x80000000) && !((rnValue + op2) & 0x80000000)) || 
                 (!(rnValue & 0x80000000) && !(op2 & 0x80000000) && ((rnValue + op2) & 0x80000000)) || 
                 // ((rnValue + op2) & 0x80000000) && (cpsr.C & 0x80000000) && !(((uint32_t)result) & 0x80000000)) ||  never happens
                 (!((rnValue + op2) & 0x80000000) && !(cpsr.C & 0x80000000) && (((uint32_t)result) & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::SBC(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint64_t result; // temporary 64 bit result for ease of calculating carry bit
    uint32_t op2;
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
        op2 = shiftResult.op2;
        result = rnValue + (~op2) + cpsr.C;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
        op2 = shiftResult.op2;
        result = rnValue + (~op2) + cpsr.C;
    } else {
        rnValue = getRegister(rn) + 8;
        op2 = shiftResult.op2;
        result = rnValue + (~op2) + cpsr.C;
    }

    setRegister(rd, (uint32_t)result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = !(result >> 32);
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // if a + b overflowed OR (a + b) + c overflowed, then result overflowed
        cpsr.V = ((rnValue & 0x80000000) && ((~op2) & 0x80000000) && !((rnValue + (~op2)) & 0x80000000)) || 
                 (!(rnValue & 0x80000000) && !((~op2) & 0x80000000) && ((rnValue + (~op2)) & 0x80000000)) || 
                 // (((rnValue + (~op2)) & 0x80000000) && (cpsr.C & 0x80000000) && !(result & 0x80000000)) || never happens
                 (!((rnValue + (~op2)) & 0x80000000) && !(cpsr.C & 0x80000000) && ((uint32_t)result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::RSC(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint64_t result; // temporary 64 bit result for ease of calculating carry bit
    uint32_t op2;
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
        op2 = shiftResult.op2;
        result = op2 + (~rnValue) + cpsr.C;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
        op2 = shiftResult.op2;
        result = op2 + (~rnValue) + cpsr.C;
    } else {
        rnValue = getRegister(rn) + 8;
        op2 = shiftResult.op2;
        result = op2 + (~rnValue) + cpsr.C;
    }

    setRegister(rd, (uint32_t)result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = !(result >> 32);
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // if a + b overflowed OR (a + b) + c overflowed, then result overflowed
        cpsr.V = ((op2 & 0x80000000) && ((~rnValue) & 0x80000000) && !((op2 + (~rnValue)) & 0x80000000)) || 
                 (!(op2 & 0x80000000) && !((~rnValue) & 0x80000000) && ((op2 + (~rnValue)) & 0x80000000)) || 
                 // (((op2 + (~rnValue)) & 0x80000000) && (cpsr.C & 0x80000000) && !(result & 0x80000000)) || never happens
                 (!((op2 + (~rnValue)) & 0x80000000) && !(cpsr.C & 0x80000000) && ((uint32_t)result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::RSB(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    uint32_t op2;
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
        op2 = shiftResult.op2;
        result = op2 - rnValue;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
        op2 = shiftResult.op2;
        result = op2 - rnValue;
    } else {
        rnValue = getRegister(rn) + 8;
        op2 = shiftResult.op2;
        result = op2 - rnValue;
    }

    setRegister(rd, result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = !(op2 < rnValue);
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        cpsr.V = (!(op2 & 0x80000000) && (rnValue & 0x80000000) && (result & 0x80000000)) || 
                 ((op2 & 0x80000000) && !(rnValue & 0x80000000) && !(result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::AND(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    if(rn != PC_REGISTER) {
        result = getRegister(rn) & shiftResult.op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        result = (getRegister(rn) + 12) & shiftResult.op2;
    } else {
        result = (getRegister(rn) + 8) & shiftResult.op2;
    }

    setRegister(rd, result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::EOR(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    if(rn != PC_REGISTER) {
        result = getRegister(rn) ^ shiftResult.op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        result = (getRegister(rn) + 12) ^ shiftResult.op2;
    } else {
        result = (getRegister(rn) + 8) ^ shiftResult.op2;
    }

    setRegister(rd, result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::ORR(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    if(rn != PC_REGISTER) {
        result = getRegister(rn) | shiftResult.op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        result = (getRegister(rn) + 12) | shiftResult.op2;
    } else {
        result = (getRegister(rn) + 8) | shiftResult.op2;
    }

    setRegister(rd, result);
    
    // updating CPSR flags 
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::MOV(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint32_t result = shiftResult.op2;
    setRegister(rd, result);
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else {} // flags not affected

    return {};
}


ARM7TDMI::Cycles ARM7TDMI::BIC(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    if(rn != PC_REGISTER) {
        result = getRegister(rn) & (~shiftResult.op2);
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        result = (getRegister(rn) + 12) & (~shiftResult.op2);
    } else {
        result = (getRegister(rn) + 8) & (~shiftResult.op2);
    }
    setRegister(rd, result);
    
    // updating CPSR flags
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


ARM7TDMI::Cycles ARM7TDMI::MVN(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint32_t result = ~shiftResult.op2;
    setRegister(rd, result);
    
    // updating CPSR flags
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


/* 
    TODO: IF S=1, with unused Rd bits=1111b, {P} opcodes (CMPP/CMNP/TSTP/TEQP):
    R15=result  ;modify PSR bits in R15, ARMv2 and below only.
    In user mode only N,Z,C,V bits of R15 can be changed.
    In other modes additionally I,F,M1,M0 can be changed.
    The PC bits in R15 are left unchanged in all modes.
*/
ARM7TDMI::Cycles ARM7TDMI::TST(uint32_t instruction) {
    // shifting operand 2
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    if(rn != PC_REGISTER) {
        result = getRegister(rn) & shiftResult.op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        result = (getRegister(rn) + 12) & shiftResult.op2;
    } else {
        result = (getRegister(rn) + 8) & shiftResult.op2;
    }
    
    // updating CPSR flags 
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { assert(false); } // flags not affected (not allowed in TST)  
    
    return {};
}


ARM7TDMI::Cycles ARM7TDMI::TEQ(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t result;
    if(rn != PC_REGISTER) {
        result = getRegister(rn) ^ shiftResult.op2;
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        result = (getRegister(rn) + 12) ^ shiftResult.op2;
    } else {
        result = (getRegister(rn) + 8) ^ shiftResult.op2;
    }
    
    // updating CPSR flags 
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        cpsr.C = shiftResult.carry;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { assert(false); } // flags not affected, not allowed in comparison ops
        
    return {};
}


ARM7TDMI::Cycles ARM7TDMI::CMP(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    // because of pipelining
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
    } else {
        rnValue = getRegister(rn) + 8;
    }
    uint32_t op2 = shiftResult.op2;
    uint32_t result = rnValue - op2;
    
    // updating CPSR flags
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        // For a subtraction, including the comparison instruction CMP,
        // C is set to 0 if the subtraction produced a borrow 
        // (that is, an unsigned underflow), and to 1 otherwise.
        // source: https://developer.arm.com/documentation/dui0801/a/Condition-Codes/Carry-flag
        cpsr.C = !(rnValue < op2);
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // *OVERFLOW* 0 1 1 (subtracting a negative is the same as adding a positive)
        // *OVERFLOW* 1 0 0 (subtracting a positive is the same as adding a negative)
        cpsr.V = (!(rnValue & 0x80000000) && (op2 & 0x80000000) && (result & 0x80000000)) || 
                 ((rnValue & 0x80000000) && !(op2 & 0x80000000) && !(result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { assert(false); }// flags not affected, not allowed in CMP
        
    return {};
}


ARM7TDMI::Cycles ARM7TDMI::CMN(uint32_t instruction) {
    AluShiftResult shiftResult = aluShift(instruction, (instruction & 0x02000000), (instruction & 0x00000010));

    uint8_t rd = (instruction & 0x0000F000) >> 12;
    uint8_t rn = (instruction & 0x000F0000) >> 16;

    // When using R15 as operand (Rm or Rn), the returned value depends 
    // on the instruction: PC+12 if I=0,R=1 (shift by register), otherwise PC+8 (shift by immediate).
    uint32_t rnValue;
    if(rn != PC_REGISTER) {
        rnValue = getRegister(rn);
    } else if(!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = getRegister(rn) + 12;
    } else {
        rnValue = getRegister(rn) + 8;
    }
    uint32_t op2 = shiftResult.op2;
    uint32_t result = rnValue + op2;

    
    // updating CPSR flags 
    if(rd != PC_REGISTER && (instruction & 0x00100000)) {
        // For an addition, including the comparison instruction CMN, 
        // C is set to 1 if the addition produced a carry (that is, an unsigned overflow),
        // and to 0 otherwise. source: https://developer.arm.com/documentation/dui0801/a/Condition-Codes/Carry-flag
        //cpsr.C = ((int32_t)(0xFFFFFFFF + op2)) < 0); 
        cpsr.C = (0xFFFFFFFF - op2) < rnValue;
        cpsr.Z = (result == 0);
        cpsr.N = (result >> 31);
        // *OVERFLOW* 1 1 -> 0
        // *OVERFLOW* 0 0 -> 1
        cpsr.V = ((rnValue & 0x80000000) && (op2 & 0x80000000) && !(result & 0x80000000)) || 
                 (!(rnValue & 0x80000000) && !(op2 & 0x80000000) && (result & 0x80000000));
    } else if(rd == PC_REGISTER && (instruction & 0x00100000)) {
        cpsr = getModeSpsr();
    } else { assert(false); } // flags not affected, not allowed in CMN

    return {};
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END OF ALU OPERATIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/



ARM7TDMI::Cycles ARM7TDMI::UNDEF(uint32_t instruction) {
    return {};
}


/*
ARM Binary Opcode Format
    |..3 ..................2 ..................1 ..................0|
    |1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
    |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| DataProc
    |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Rs___|0|Typ|1|__Rm___| DataProc
    |_Cond__|0_0_1|___Op__|S|__Rn___|__Rd___|_Shift_|___Immediate___| DataProc
    |_Cond__|0_0_1_1_0|P|1|0|_Field_|__Rd___|_Shift_|___Immediate___| PSR Imm
    |_Cond__|0_0_0_1_0|P|L|0|_Field_|__Rd___|0_0_0_0|0_0_0_0|__Rm___| PSR Reg
    |_Cond__|0_0_0_1_0_0_1_0_1_1_1_1_1_1_1_1_1_1_1_1|0_0|L|1|__Rn___| BX,BLX
    |_Cond__|0_0_0_0_0_0|A|S|__Rd___|__Rn___|__Rs___|1_0_0_1|__Rm___| Multiply
    |_Cond__|0_0_0_0_1|U|A|S|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| MulLong
    |_Cond__|0_0_0_1_0|Op_|0|Rd/RdHi|Rn/RdLo|__Rs___|1|y|x|0|__Rm___| MulHalfARM9
    |_Cond__|0_0_0|P|U|0|W|L|__Rn___|__Rd___|0_0_0_0|1|S|H|1|__Rm___| TransReg10
    |_Cond__|0_0_0|P|U|1|W|L|__Rn___|__Rd___|OffsetH|1|S|H|1|OffsetL| TransImm10
    |_Cond__|0_1_0|P|U|B|W|L|__Rn___|__Rd___|_________Offset________| TransImm9
    |_Cond__|0_1_1|P|U|B|W|L|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| TransReg9
    |_Cond__|0_1_1|________________xxx____________________|1|__xxx__| Undefined
    |_Cond__|1_0_0|P|U|S|W|L|__Rn___|__________Register_List________| BlockTrans
    |_Cond__|1_0_1|L|___________________Offset______________________| B,BL,BLX
    |_Cond__|1_1_1_1|_____________Ignored_by_Processor______________| SWI           
*/  

// TODO: find a way to write a decoder that is as FAST and SIMPLE as possible
ARM7TDMI::Cycles ARM7TDMI::executeInstruction(uint32_t instruction) {
    // highest possible specificity to lowest specifity
    switch(instruction & 0x010E00000) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4: 
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8: 
        case 0x9: 
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
            break;
    }
    return {};
}


// Comment documentation sourced from the ARM7TDMI Data Sheet. 
// TODO: potential optimization (?) only return carry bit and just shift the op2 in the instruction itself
ARM7TDMI::AluShiftResult ARM7TDMI::aluShift(uint32_t instruction, bool i, bool r) {
    if(i) { // shifted immediate value as 2nd operand
        /*
            The immediate operand rotate field is a 4 bit unsigned integer 
            which specifies a shift operation on the 8 bit immediate value. 
            This value is zero extended to 32 bits, and then subject to a 
            rotate right by twice the value in the rotate field.
        */
        uint32_t rm = instruction & 0x000000FF;
        uint8_t is = (instruction & 0x00000F00) >> 7U;
        uint32_t op2 = aluShiftRor(rm, is % 32);
        uint8_t carry;
        // carry out bit is the least significant discarded bit of rm
        if(is > 0) {
            carry = (rm >> (is - 1)); 
        }
        return {op2, carry};
    }

    /* ~~~~~~~~~ else: shifted register value as 2nd operand ~~~~~~~~~~ */
    uint8_t shiftType = (instruction & 0x00000060) >> 5U;
    uint32_t op2;
    uint8_t rmIndex = instruction & 0x0000000F;
    uint32_t rm = getRegister(rmIndex);
    // see comment in opcode functions for explanation why we're doing this
    if(rmIndex != PC_REGISTER) {
        // do nothing
    } else if(!i && r) {
        rm += 12;
    } else {
        rm += 8;
    }

    uint32_t shiftAmount;
    uint8_t carry;

    if(r) { // register as shift amount
        uint8_t rsIndex = (instruction & 0x00000F00) >> 8U;
        assert(rsIndex != 15);
        shiftAmount = getRegister(rsIndex) & 0x000000FF;
    } else { // immediate as shift amount
        shiftAmount = instruction & 0x00000F80 >> 7U;
    }

    bool immOpIsZero = r ? false : shiftAmount == 0;

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
            op2 = aluShiftRor(rm, shiftAmount % 32);
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


// TODO: make sure all these shift functions are correct
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
    if(cpsr.Mode == USER) {
        return registers[index];
    } else if(cpsr.Mode == FIQ) {
        return *(fiqRegisters[index]);
    } else if(cpsr.Mode == IRQ) {
        return *(irqRegisters[index]);
    } else if(cpsr.Mode == SUPERVISOR) {
        return *(svcRegisters[index]);
    } else if(cpsr.Mode == ABORT) {
        return *(abtRegisters[index]);
    } else if(cpsr.Mode == UNDEFINED) {
        return *(undRegisters[index]);
    } else {
        return registers[index];
    }
}


void ARM7TDMI::setRegister(uint8_t index, uint32_t value) {
    if(cpsr.Mode == USER) {
        registers[index] = value;
    } else if(cpsr.Mode == FIQ) {
        *(fiqRegisters[index]) = value;
    } else if(cpsr.Mode == IRQ) {
        *(irqRegisters[index]) = value;
    } else if(cpsr.Mode == SUPERVISOR) {
        *(svcRegisters[index]) = value;
    } else if(cpsr.Mode == ABORT) {
        *(abtRegisters[index]) = value;
    } else if(cpsr.Mode == UNDEFINED) {
        *(undRegisters[index]) = value;
    } else {
        registers[index] = value;
    }
}
