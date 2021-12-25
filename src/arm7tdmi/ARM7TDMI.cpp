
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