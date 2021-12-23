
#include "ARM7TDMI.h"

#include <bit>
#include <bitset>
#include <iostream>
#include <type_traits>
#include <string.h>

#include "../memory/Bus.h"
#include "../Timer.h"
#include "../Debugger.h"

#include "assert.h"


void ARM7TDMI::initializeWithRom() {
    switchToMode(SYSTEM);
    cpsr.Mode = SYSTEM;
    cpsr.T = 0; // set CPU to ARM state
    setRegister(PC_REGISTER, BOOT_LOCATION); 
    currentPcAccessType = BRANCH;
    setRegister(SP_REGISTER, 0x03007F00); // stack pointer
    r13_svc = 0x03007FE0; // SP_svc=03007FE0h
    r13_irq = 0x03007FA0; // SP_irq=03007FA0h

    bus->resetCycleCountTimeline();
    uint32_t pcAddress = getRegister(PC_REGISTER);
    currInstruction = bus->read32(pcAddress, Bus::CycleType::NONSEQUENTIAL);
    // emulate filling the pipeline
    bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 4, 32);
    bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 8, 32);
}

ARM7TDMI::~ARM7TDMI() {}

uint32_t ARM7TDMI::getCurrentInstruction() {
    return currInstruction;
}


uint32_t ARM7TDMI::step() {
    // TODO: give this method a better name
    bus->resetCycleCountTimeline();

    if((bus->iORegisters[Bus::IORegister::IME] & 0x1) && 
       (!cpsr.I) &&
       ((bus->iORegisters[Bus::IORegister::IE] & bus->iORegisters[Bus::IORegister::IF]) || 
       ((bus->iORegisters[Bus::IORegister::IE + 1] & 0x3F) & (bus->iORegisters[Bus::IORegister::IF + 1] & 0x3F)))) {
        //Debugger::stepMode = true;
        // interrupts is enabled
        irq();
    }

    if (!cpsr.T) {  // check state bit, is CPU in ARM state?

        uint8_t cond = (currInstruction & 0xF0000000) >> 28;

        // increment PC
        setRegister(PC_REGISTER, getRegister(PC_REGISTER) + 4);
        if(conditionalHolds(cond)) { 
            currentPcAccessType = 
                armLut[((currInstruction & 0x0FF00000) >> 16) | 
                       ((currInstruction & 0x0F0) >> 4)](currInstruction, this);
        } else {
            currentPcAccessType = SEQUENTIAL;
        }
        
    } else {  // THUMB state
        setRegister(PC_REGISTER, getRegister(PC_REGISTER) + 2);
        currentPcAccessType = thumbLut[(currInstruction >> 6)](currInstruction, this);
    }

    getNextInstruction(currentPcAccessType);

    // TODO: just return one cycle per instr for now
    return 1;
}

inline
void ARM7TDMI::getNextInstruction(FetchPCMemoryAccess currentPcAccessType) {
    currInstrAddress = getRegister(PC_REGISTER);
    if(cpsr.T) {
        currInstruction = bus->read16(currInstrAddress, Bus::CycleType::NONSEQUENTIAL);
    } else {
        currInstruction = bus->read32(currInstrAddress, Bus::CycleType::NONSEQUENTIAL);
    }
    return;
}

inline
void ARM7TDMI::irq() {
    uint32_t returnAddr = getRegister(PC_REGISTER) + 4;
    switchToMode(Mode::IRQ);
    // switch to ARM mode
    *currentSpsr = cpsr;
    cpsr.T = 0;
    cpsr.I = 1; 
    cpsr.Mode = Mode::IRQ;
    setRegister(PC_REGISTER, 0x18);
    setRegister(LINK_REGISTER, returnAddr);
    getNextInstruction(FetchPCMemoryAccess::BRANCH);
}

void ARM7TDMI::queueInterrupt(Interrupt interrupt) {
    bus->iORegisters[Bus::IORegister::IF] |= ((uint16_t)interrupt & 0xFF);
    bus->iORegisters[Bus::IORegister::IF + 1] |= (((uint16_t)interrupt >> 8) & 0xFF);
}

void ARM7TDMI::connectBus(std::shared_ptr<Bus> bus) { 
    this->bus = bus; 
}


inline
bool ARM7TDMI::conditionalHolds(uint8_t cond) {
    switch(cond) {
        case Condition::EQ: {
            return cpsr.Z;
        }
        case Condition::NE: {
            return !cpsr.Z;
        }
        case Condition::CS: {
            return cpsr.C;
        }
        case Condition::CC: {
            return !cpsr.C;
        }
        case Condition::MI: {
            return cpsr.N;
        }
        case Condition::PL: {
            return !cpsr.N;
        }
        case Condition::VS: {
            return cpsr.V;
        }
        case Condition::VC: {
            return !cpsr.V;
        }
        case Condition::HI: {
            return cpsr.C && !cpsr.Z;
        }
        case Condition::LS: {
            return !cpsr.C || cpsr.Z;
        }
        case Condition::GE: {
            return cpsr.N == cpsr.V;
        }
        case Condition::LT: {
            return cpsr.N != cpsr.V;
        }
        case Condition::GT: {
            return !cpsr.Z && (cpsr.N == cpsr.V);
        }
        case Condition::LE: {
            return cpsr.Z || (cpsr.N != cpsr.V);
        }
        case Condition::AL: {
            return true;
        }
        case Condition::NV: {
           return false;       
        }
        default: {
            assert(false);
            return false;
        }
    }
}


// Comment documentation sourced from the ARM7TDMI Data Sheet.
// TODO: potential optimization (?) only return carry bit and just shift the op2
// in the instruction itself
inline
ARM7TDMI::AluShiftResult ARM7TDMI::aluShift(uint32_t instruction, bool i,
                                            bool r) {
    if (i) {  // shifted immediate value as 2nd operand
        /*
            The immediate operand rotate field is a 4 bit unsigned integer
            which specifies a shift operation on the 8 bit immediate value.
            This value is zero extended to 32 bits, and then subject to a
            rotate right by twice the value in the rotate field.
        */

        uint32_t imm = instruction & 0x000000FF;
        uint8_t is = (instruction & 0x00000F00) >> 7U;
        uint32_t op2 = aluShiftRor(imm, is % 32);

        // carry out bit is the least significant discarded bit of rm
        if (is > 0) {
            carryBit = (imm >> (is - 1)) & 0x1;
        } else {
            carryBit = cpsr.C;
        }
        return {op2, carryBit};
    }

    /* ~~~~~~~~~ else: shifted register value as 2nd operand ~~~~~~~~~~ */
    uint8_t shiftType = (instruction & 0x00000060) >> 5U;
    uint32_t op2;
    uint8_t rmIndex = instruction & 0x0000000F;
    uint32_t rm = getRegister(rmIndex);
    // see comment in opcode functions for explanation why we're doing this
    if (rmIndex != PC_REGISTER) {
        // do nothing
    } else if (!i && r) {
        rm += 8;
    } else {
        rm += 4;
    }

    uint32_t shiftAmount;

    if (r) {  // register as shift amount
        uint8_t rsIndex = (instruction & 0x00000F00) >> 8U;
        assert(rsIndex != 15);
        shiftAmount = getRegister(rsIndex) & 0x000000FF;
        if(shiftAmount == 0) {
            return {rm, cpsr.C};
        }
    } else {  // immediate as shift amount
        shiftAmount = (instruction & 0x00000F80) >> 7U;
    }

    bool immOpIsZero = r ? false : shiftAmount == 0;

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
            op2 = aluShiftLsl(rm, shiftAmount);
            carryBit = (shiftAmount > 32) ? 0 : ((rm >> (32 - shiftAmount)) & 1);
        } else {  // no op performed, carry flag stays the same
            op2 = rm;
            carryBit = cpsr.C;
        }
    } else if (shiftType == 1) {  // Logical Shift Right
        /*
            A logical shift right (LSR) is similar, but the contents
            of Rm are moved to less significant positions in the result
        */
        if (!immOpIsZero) {
            op2 = aluShiftLsr(rm, shiftAmount);
            carryBit = (shiftAmount > 32) ? 0 : ((rm >> (shiftAmount - 1)) & 1);
        } else {
            /*
                The form of the shift field which might be expected to
                correspond to LSR #0 is used to encode LSR #32, which has a
                zero result with bit 31 of Rm as the carry output
            */
            op2 = 0;
            carryBit = rm >> 31;
        }
    } else if (shiftType == 2) {  // Arithmetic Shift Right
        /*
            An arithmetic shift right (ASR) is similar to logical shift right,
            except that the high bits are filled with bit 31 of Rm instead of
           zeros. This preserves the sign in 2's complement notation
        */
        if (!immOpIsZero) {
            op2 = aluShiftAsr(rm, shiftAmount);
            carryBit = (shiftAmount >= 32) ? (rm & 0x80000000) : ((rm >> (shiftAmount - 1)) & 1);
        } else {
            /*
                The form of the shift field which might be expected to give ASR
               #0 is used to encode ASR #32. Bit 31 of Rm is again used as the
               carry output, and each bit of operand 2 is also equal to bit 31
               of Rm.
            */
            op2 = aluShiftAsr(rm, 32);
            carryBit = rm >> 31;
        }
    } else {  // Rotating Shift
        /*
            Rotate right (ROR) operations reuse the bits which “overshoot”
            in a logical shift right operation by reintroducing them at the
            high end of the result, in place of the zeros used to fill the high
            end in logical right operation
        */
        if (!immOpIsZero) {
            op2 = aluShiftRor(rm, shiftAmount % 32);
            carryBit = (rm >> ((shiftAmount % 32) - 1)) & 1;
        } else {
            /*
                The form of the shift field which might be expected to give ROR
               #0 is used to encode a special function of the barrel shifter,
                rotate right extended (RRX). This is a rotate right by one bit
               position of the 33 bit quantity formed by appending the CPSR C
               flag to the most significant end of the contents of Rm as shown
            */
            op2 = rm >> 1;
            op2 = op2 | (((uint32_t)cpsr.C) << 31);
            carryBit = rm & 1;
        }
    }
    return {op2, carryBit};
}


ARM7TDMI::ProgramStatusRegister *ARM7TDMI::getCurrentModeSpsr() {
    return currentSpsr;
}

ARM7TDMI::ProgramStatusRegister ARM7TDMI::getCpsr() {
    return cpsr;
}


uint32_t ARM7TDMI::getRegister(uint8_t index) { 
    return *(registers[index]); 
}

uint32_t ARM7TDMI::getUserRegister(uint8_t index) {
    return *(userRegisters[index]); 
}

void ARM7TDMI::setRegister(uint8_t index, uint32_t value) {
    *(registers[index]) = value;
}

void ARM7TDMI::setUserRegister(uint8_t index, uint32_t value) {
    *(userRegisters[index]) = value;
}

inline
uint8_t ARM7TDMI::getOpcode(uint32_t instruction) {
    return (instruction & 0x01E00000) >> 21;
}

inline
bool ARM7TDMI::sFlagSet(uint32_t instruction) {
    return (instruction & 0x00100000);
}


void ARM7TDMI::setCurrInstruction(uint32_t instruction) {
    currInstruction = instruction;
}


/*
  Bit   Expl.
  31    N - Sign Flag       (0=Not Signed, 1=Signed)               ;\
  30    Z - Zero Flag       (0=Not Zero, 1=Zero)                   ; Condition
  29    C - Carry Flag      (0=Borrow/No Carry, 1=Carry/No Borrow) ; Code Flags
  28    V - Overflow Flag   (0=No Overflow, 1=Overflow)            ;/
  27    Q - Sticky Overflow (1=Sticky Overflow, ARMv5TE and up only)
  26-25 Reserved            (For future use) - Do not change manually!
  24    J - Jazelle Mode    (1=Jazelle Bytecode instructions) (if supported)
  23-10 Reserved            (For future use) - Do not change manually!
  9     E - Endian          (... Big endian)                  (ARM11 ?)
  8     A - Abort disable   (1=Disable Imprecise Data Aborts) (ARM11 only)
  7     I - IRQ disable     (0=Enable, 1=Disable)                     ;\
  6     F - FIQ disable     (0=Enable, 1=Disable)                     ; Control
  5     T - State Bit       (0=ARM, 1=THUMB) - Do not change manually!; Bits
  4-0   M4-M0 - Mode Bits   (See below)                               ;/
eturn value;

*/