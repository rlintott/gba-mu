#include "ARM7TDMI.h"
#include "Bus.h"
#include <bit>
#include <bitset>
#include <iostream>
#include "assert.h"
#include <bitset>


ARM7TDMI::ARM7TDMI() {
    switchToMode(SUPERVISOR);
    cpsr.T = 0; // set CPU to ARM state
    setRegister(PC_REGISTER, BOOT_LOCATION);
}


ARM7TDMI::~ARM7TDMI() {
}


void ARM7TDMI::step() {
    // read from program counter
    uint32_t instruction = bus->read32(getRegister(PC_REGISTER));
    DEBUG(std::bitset<32>(instruction).to_string() << std::endl);

    if(!cpsr.T) { // check state bit, is CPU in ARM state?
        ArmOpcodeHandler handler = decodeArmInstruction(instruction);
        Cycles cycles = handler(instruction, this);
        // increment PC
        setRegister(PC_REGISTER, getRegister(PC_REGISTER) + 4); 
    } else { // THUMB state

    }
}

void ARM7TDMI::connectBus(Bus* bus) {
    this->bus = bus;
}


void ARM7TDMI::switchToMode(Mode mode) {
    cpsr.Mode = mode;
    switch(mode) {
        case USER: {
            currentSpsr = &cpsr;
            registers[8] = &r8;
            registers[9] = &r9;
            registers[10] = &r10;
            registers[11] = &r11;
            registers[12] = &r12;
            registers[13] = &r13;
            registers[14] = &r14;
            break;
        }
        case FIQ: {
            currentSpsr = &SPSR_fiq;
            registers[8] = &r8_fiq;
            registers[9] = &r9_fiq;
            registers[10] = &r10_fiq;
            registers[11] = &r11_fiq;
            registers[12] = &r12_fiq;
            registers[13] = &r13_fiq;
            registers[14] = &r14_fiq;
            break;
        }
        case IRQ: {
            currentSpsr = &SPSR_irq;
            registers[13] = &r13_irq;
            registers[14] = &r14_irq;
            break;
        }
        case SUPERVISOR: {
            currentSpsr = &SPSR_svc;
            registers[13] = &r13_svc;
            registers[14] = &r14_svc;
            break;
        }
        case ABORT: {
            currentSpsr = &SPSR_abt;
            registers[13] = &r13_abt;
            registers[14] = &r14_abt;
            break;
        }
        case UNDEFINED: {
            currentSpsr = &SPSR_und;
            registers[13] = &r13_abt;
            registers[14] = &r14_abt;
            break;
        }
        case SYSTEM: {
            currentSpsr = &SPSR_svc;
            registers[13] = &r13_svc; // ? TODO check
            registers[14] = &r14_svc; // ?
            break;
        }
    }
}


void ARM7TDMI::transferToPsr(uint32_t value, uint8_t field, ProgramStatusRegister* psr) {
    if(field & 0b1000) {
        // TODO: is this correct? it says   f =  write to flags field     Bit 31-24 (aka _flg)
        psr->N = (bool)(value & 0x80000000);
        psr->Z = (bool)(value & 0x40000000);
        psr->C = (bool)(value & 0x20000000);
        psr->V = (bool)(value & 0x10000000);
        psr->Q = (bool)(value & 0x08000000);
        psr->Reserved = (psr->Reserved & 0b0001111111111111111) | 
                        ((value & 0x07000000) >> 8);
    }
    if(field & 0b0100) {
        // reserved, don't change
    }
    if(field & 0b0010) {
        // reserverd don't change
    }
    if(cpsr.Mode != USER) {
        if(field & 0b0001) {
            psr->I = (bool)(value & 0x00000080);
            psr->F = (bool)(value & 0x00000040);
             // t bit may not be changed, for THUMB/ARM switching use BX instruction.
            assert(!(bool)(value & 0x00000020));
            psr->T = (bool)(value & 0x00000020);
            psr->Mode = 0 | (value & 0x00000010) | 
                            (value & 0x00000008) | 
                            (value & 0x00000004) | 
                            (value & 0x00000002) | 
                            (value & 0x00000001);
        }
    }
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

decoding from highest to lowest specifity to ensure corredct opcode parsed

    case: 000

        1:  xxxx0001001011111111111100x1xxxx    BX,BLX
        2:  xxxx00010x00xxxxxxxx00001001xxxx    TransSwp12  (15)
        3:  xxxx00010xx0xxxxxxxx00000000xxxx    PSR Reg     (14)
        4:  xxxx000xx0xxxxxxxxxx00001xx1xxxx    TransReg10  (10)
        5:  xxxx000000xxxxxxxxxxxxxx1001xxxx    Multiply    (10)
        6:  xxxx00001xxxxxxxxxxxxxxx1001xxxx    MulLong     (9)
        7:  xxxx000xx1xxxxxxxxxxxxxx1xx1xxxx    TransImm10  (6)
        9:  xxxx000xxxxxxxxxxxxxxxxx0xx1xxxx    DataProc    (5)
        10: xxxx000xxxxxxxxxxxxxxxxxxxx0xxxx    DataProc    (4)

    case 001:

        8:  xxxx00110x10xxxxxxxxxxxxxxxxxxxx    PSR Imm     (7)
        16: xxxx001xxxxxxxxxxxxxxxxxxxxxxxxx    DataProc    (3)

    case 100:

        17: xxxx100xxxxxxxxxxxxxxxxxxxxxxxxx    BlockTrans  (3)

    case 101:

        14: xxxx101xxxxxxxxxxxxxxxxxxxxxxxxx    B,BL,BLX    (3)

    case 111:
    
        12: xxxx1111xxxxxxxxxxxxxxxxxxxxxxxx    SWI         (4)
    
    case 011:

        11: xxxx011xxxxxxxxxxxxxxxxxxxx0xxxx    TransReg9   (4)
        13: xxxx011xxxxxxxxxxxxxxxxxxxx1xxxx    Undefined   (4)
         
*/  
// TODO: use hex values to make it more concise 
ARM7TDMI::ArmOpcodeHandler ARM7TDMI::decodeArmInstruction(uint32_t instruction) {
    switch(instruction & 0b00001110000000000000000000000000) { // mask 1
        case 0b00000000000000000000000000000000: {
            if((instruction & 0b00001111111111111111111111010000) == 
                              0b00000001001011111111111100010000) { // BX,BLX    
            }
            else if((instruction & 0b00001111101100000000111111110000) ==
                                   0b00000001000000000000000010010000) { // TransSwp12 
            
            }
            else if((instruction & 0b00001111100100000000111111110000) == 
                                   0b00000001000000000000000000000000) { // PSR Reg
               return ArmOpcodeHandlers::psrHandler; 
            }
            else if((instruction & 0b00001110010000000000111110010000) == 
                                   0b00000000000000000000000010010000) { // TransReg10
            
            }
            else if((instruction & 0b00001111110000000000000011110000) == 
                                   0b00000000000000000000000010010000) { // Multiply
                return ArmOpcodeHandlers::multiplyHandler;
            }
            else if((instruction & 0b00001111100000000000000011110000) == 
                                   0b00000000100000000000000010010000) { // MulLong
            
            }
            else if((instruction & 0b00001110010000000000000010010000) == 
                                   0b00000000010000000000000010010000) { // TransImm10
            
            }
            else { // dataProc
                return ArmOpcodeHandlers::aluHandler;
            }
            break;
        }
        case 0b00000010000000000000000000000000: {
            if((instruction & 0b00001111101100000000000000000000) == 
                              0b00000011001000000000000000000000) { // PSR Imm
                return ArmOpcodeHandlers::psrHandler;
            }
            else { // DataProc 
                return ArmOpcodeHandlers::aluHandler;
            }
        }
        case 0b00001000000000000000000000000000: {
            break;
        }
        case 0b00001010000000000000000000000000: {
            break;
        }
        case 0b00001110000000000000000000000000: {
            break;
        }
        case 0b00000110000000000000000000000000: {
            if((instruction & 0b00001110000000000000000000010000) == 
                              0b00000110000000000000000000000000) { // TransReg9
            }
            else { // Undefined 
            }
            break;
        }
        default: {
            return ArmOpcodeHandlers::undefinedOpHandler; 
        }
    }
    return ArmOpcodeHandlers::undefinedOpHandler;
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
        uint32_t rm = getRm(instruction);
        uint8_t is = (instruction & 0x00000F00) >> 7U;
        uint32_t op2 = aluShiftRor(rm, is % 32);
        // carry out bit is the least significant discarded bit of rm
        if(is > 0) {
            carryBit = (rm >> (is - 1)); 
        }
        return {op2, carryBit};
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
            carryBit = (rm >> (32 - shiftAmount)) & 1;
        } else { // no op performed, carry flag stays the same
            op2 = rm;
            carryBit = cpsr.C;
        }
    }
    else if(shiftType == 1) { // Logical Shift Right
        /*
            A logical shift right (LSR) is similar, but the contents 
            of Rm are moved to less significant positions in the result    
        */
        if(!immOpIsZero) {
            op2 = aluShiftLsr(rm, shiftAmount);
            carryBit = (rm >> (shiftAmount - 1)) & 1;
        } else {
            /*
                The form of the shift field which might be expected to 
                correspond to LSR #0 is used to encode LSR #32, which has a 
                zero result with bit 31 of Rm as the carry output
            */
            op2 = 0;
            carryBit = rm >> 31;
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
            carryBit = rm >> 31;
        } else {
            /*
                The form of the shift field which might be expected to give ASR #0 
                is used to encode ASR #32. Bit 31 of Rm is again used as the carry output, 
                and each bit of operand 2 is also equal to bit 31 of Rm. 
            */
            op2 = aluShiftAsr(rm, 32);
            carryBit = rm >> 31;
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
            carryBit = (rm >> (shiftAmount - 1)) & 1;
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
            carryBit = rm & 1;
        }
    }

    return {op2, carryBit};
}


// TODO: separate these utility functions into a different file
// TODO: make sure all these shift functions are correct
uint32_t ARM7TDMI::aluShiftLsl(uint32_t value, uint8_t shift) {
    return value << shift;
}


uint32_t ARM7TDMI::aluShiftLsr(uint32_t value, uint8_t shift) {
    return value >> shift;
}


uint32_t ARM7TDMI::aluShiftAsr(uint32_t value, uint8_t shift) {
    return value < 0 ? ~(~value >> shift) : value >> shift ;
}


uint32_t ARM7TDMI::aluShiftRor(uint32_t value, uint8_t shift) {
    assert(shift < 32U);
    return (value >> shift) | (value << (-shift & 31U));
}


uint32_t ARM7TDMI::aluShiftRrx(uint32_t value, uint8_t shift, ARM7TDMI* cpu) {
    assert(shift < 32U);
    uint32_t rrxMask = (cpu->cpsr).C;
    return ((value >> shift) | (value << (-shift & 31U))) | (rrxMask << 31U);
}


ARM7TDMI::ProgramStatusRegister* ARM7TDMI::getCurrentModeSpsr() {
    return currentSpsr;
}


uint32_t ARM7TDMI::getRegister(uint8_t index) {
    return *(registers[index]);
}


void ARM7TDMI::setRegister(uint8_t index, uint32_t value) {
    *(registers[index]) = value;
}


bool ARM7TDMI::aluSetsZeroBit(uint32_t value) {
    return value == 0;
}


bool ARM7TDMI::aluSetsSignBit(uint32_t value) {
    return value >> 31;
}


bool ARM7TDMI::aluSubtractSetsCarryBit(uint32_t rnValue, uint32_t op2) {
    return !(rnValue < op2);
}


bool ARM7TDMI::aluSubtractSetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result) {
    // todo: maybe there is a more efficient way to do this
    return (!(rnValue & 0x80000000) && (op2 & 0x80000000) && (result & 0x80000000)) || 
            ((rnValue & 0x80000000) && !(op2 & 0x80000000) && !(result & 0x80000000));
}


bool ARM7TDMI::aluAddSetsCarryBit(uint32_t rnValue, uint32_t op2) {
    return (0xFFFFFFFFU - op2) < rnValue;
}


bool ARM7TDMI::aluAddSetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result) {
    // todo: maybe there is a more efficient way to do this
    return ((rnValue & 0x80000000) && (op2 & 0x80000000) && !(result & 0x80000000)) || 
            (!(rnValue & 0x80000000) && !(op2 & 0x80000000) && (result & 0x80000000));
}


bool ARM7TDMI::aluAddWithCarrySetsCarryBit(uint64_t result) {
    return result >> 32;
}


bool ARM7TDMI::aluAddWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result, ARM7TDMI* cpu) {
    // todo: maybe there is a more efficient way to do this
    return   ((rnValue & 0x80000000) && (op2 & 0x80000000) && !((rnValue + op2) & 0x80000000)) || 
             (!(rnValue & 0x80000000) && !(op2 & 0x80000000) && ((rnValue + op2) & 0x80000000)) || 
            // ((rnValue + op2) & 0x80000000) && (cpsr.C & 0x80000000) && !(((uint32_t)result) & 0x80000000)) ||  never happens
             (!((rnValue + op2) & 0x80000000) && !(cpu->cpsr.C & 0x80000000) && ((result) & 0x80000000));
}


bool ARM7TDMI::aluSubWithCarrySetsCarryBit(uint64_t result) {
    return !(result >> 32);
}


bool ARM7TDMI::aluSubWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result, ARM7TDMI* cpu) {
    // todo: maybe there is a more efficient way to do this
    return ((rnValue & 0x80000000) && ((~op2) & 0x80000000) && !((rnValue + (~op2)) & 0x80000000)) || 
            (!(rnValue & 0x80000000) && !((~op2) & 0x80000000) && ((rnValue + (~op2)) & 0x80000000)) || 
             // (((rnValue + (~op2)) & 0x80000000) && (cpsr.C & 0x80000000) && !(result & 0x80000000)) || never happens
            (!((rnValue + (~op2)) & 0x80000000) && !(cpu->cpsr.C & 0x80000000) && (result & 0x80000000));
}

// not guaranteed to always be rn, check the spec first
uint8_t ARM7TDMI::getRn(uint32_t instruction) {
    return (instruction & 0x000F0000) >> 16;
}

// not guaranteed to always be rd, check the spec first
uint8_t ARM7TDMI::getRd(uint32_t instruction) {
    return (instruction & 0x0000F000) >> 12;
}

// not guaranteed to always be rs, check the spec first
uint8_t ARM7TDMI::getRs(uint32_t instruction) {
    return (instruction & 0x00000F00) >> 8;
}


uint8_t ARM7TDMI::getRm(uint32_t instruction) {
    return (instruction & 0x0000000F);
}


uint8_t ARM7TDMI::getOpcode(uint32_t instruction) {
    return (instruction & 0x01E00000) >> 21;
}

bool ARM7TDMI::sFlagSet(uint32_t instruction) {
    return (instruction & 0x00100000);
}


uint32_t ARM7TDMI::psrToInt(ProgramStatusRegister psr) {
    return 0 |  (((uint32_t)psr.N) << 31)        | 
                (((uint32_t)psr.Z) << 30)        | 
                (((uint32_t)psr.C) << 29)        |
                (((uint32_t)psr.V) << 28)        |
                (((uint32_t)psr.Q) << 27)        |
                (((uint32_t)psr.Reserved) << 26) | 
                (((uint32_t)psr.I) << 7)         |
                (((uint32_t)psr.F) << 6)         |
                (((uint32_t)psr.T) << 5)         |
                (((uint32_t)psr.Mode) << 0);
} 
