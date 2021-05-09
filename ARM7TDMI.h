#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <iostream>

class Bus;

class ARM7TDMI {

public: 
    ARM7TDMI();
    ~ARM7TDMI();

private:

    // general purpose registers 
    std::array<uint32_t, 16> registers = {
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,    // stack pointer
        0x0,    // link register
        0x0     // program counter
    };

    //  8 - 14 "banked" registers for mode FIQ
    std::array<uint32_t, 7> fiqRegisters = {
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0
    };

    //  13 - 14 "banked" registers for mode IRQ
    std::array<uint32_t, 2> irqRegisters = {
        0x0,
        0x0,
    };

    //  13 - 14 "banked" registers for mode SVC
    std::array<uint32_t, 2> svcRegisters = {
        0x0,
        0x0,
    };

    //  13 - 14 "banked" registers for mode ABT
    std::array<uint32_t, 2> abtRegisters = {
        0x0,
        0x0,
    };

    //  13 - 14 "banked" registers for mode UND
    std::array<uint32_t, 2> undRegisters = {
        0x0,
        0x0,
    };

    // struct representing current program status register (CPSR)
    struct CPSR {
        uint8_t Mode : 5;        //  M4-M0 - Mode Bits
        uint8_t T : 1;          // T - State Bit       (0=ARM, 1=THUMB) - Do not change manually!
        uint8_t F : 1;          // F - FIQ disable     (0=Enable, 1=Disable)    
        uint8_t I : 1;          // I - IRQ disable     (0=Enable, 1=Disable)
        uint32_t Reserved : 19; // Reserved            (For future use) - Do not change manually!
        uint8_t Q : 1;          // Q - Sticky Overflow (1=Sticky Overflow, ARMv5TE and up only)
        uint8_t V : 1;          // V - Overflow Flag   (0=No Overflow, 1=Overflow)  
        uint8_t C : 1;          // C - Carry Flag      (0=Borrow/No Carry, 1=Carry/No Borrow
        uint8_t Z : 1;          //Z - Zero Flag       (0=Not Zero, 1=Zero)   
        uint8_t N : 1;          // N - Sign Flag       (0=Not Signed, 1=Signed)               
    };

    enum AluOperationType {
        LOGICAL,
        ARITHMETIC,
        COMPARISON,
    };

    // helper table to map alu operations to type
    std::array<AluOperationType, 16> aluOperationTypeTable = {
        LOGICAL, 
        LOGICAL,
        ARITHMETIC,
        ARITHMETIC,
        ARITHMETIC,
        ARITHMETIC,
        ARITHMETIC,
        ARITHMETIC,
        COMPARISON,
        COMPARISON,
        COMPARISON,
        COMPARISON,
        LOGICAL,
        LOGICAL,
        LOGICAL,
        LOGICAL
    };

    enum Mode {
        USER = 16,
        FIQ = 17,
        IRQ = 18,
        SUPERVISOR = 19,
        ABORT = 23,
        UNDEFINED = 27,
        SYSTEM = 31
    };

    enum AsmOpcodeType {
        DATA_PROCESSING_IMM, 
        DATA_PROCESSING_REG_SHIFT_IMM,
        DATA_PROCESSING_REG_SHIFT_REG,
        UNDEFINED_OPCODE
    };

    CPSR cpsr = {0,0,0,0,0,0,0,0,0,0};
    CPSR SPSR_fiq = {0,0,0,0,0,0,0,0,0,0};
    CPSR SPSR_svc = {0,0,0,0,0,0,0,0,0,0};
    CPSR SPSR_abt = {0,0,0,0,0,0,0,0,0,0};
    CPSR SPSR_irq = {0,0,0,0,0,0,0,0,0,0};
    CPSR SPSR_und = {0,0,0,0,0,0,0,0,0,0};


    // struct representing the number of cycles an operation will take
    struct Cycles {
        uint8_t nonSequentialCycles : 8;
        uint8_t sequentialCycles : 8;
        uint8_t internalCycles : 8;
        uint8_t waitState : 8;
    };

    struct Instruction {
        std::string name;
        Cycles (ARM7TDMI::*execute)(uint32_t) = nullptr;
    };

    std::vector<Instruction> dataProccessingOpcodes;

    Instruction undefinedOpcode;

    Bus *bus;

    AsmOpcodeType getAsmOpcodeType(uint32_t instruction);

    Cycles AND(uint32_t instruction);
    Cycles UNDEF(uint32_t instruction);

    uint32_t aluShiftLsl(uint32_t value, uint8_t shift);
    uint32_t aluShiftLsr(uint32_t value, uint8_t shift);
    uint32_t aluShiftAsr(uint32_t value, uint8_t shift);
    uint32_t aluShiftRor(uint32_t value, uint8_t shift);
    uint32_t aluShiftRrx(uint32_t value, uint8_t shift);

    AluOperationType getAluOperationType(uint32_t instruction);

public:

    void executeInstructionCycle();

    void clock();

    // CPU exceptions
    void irq();
    void firq();
    void undefinedInstruction();
    void reset();

    Instruction decodeInstruction(uint32_t rawInstruction);

    // accounts for modes, ex in IRQ mode, getting register 14 will return value of R14_irq
    uint32_t getRegister(uint8_t index);

    // accounts for modes, ex in IRQ mode, setting register 14 will set value of R14_irq
    void setRegister(uint8_t index, uint32_t value);

    // shifts the second operand according to ALU logic. NOTE: This function may modify cpsr.C flag
    uint32_t aluShift(uint32_t instruction, bool i, bool r);

    void aluUpdateCpsrFlags(AluOperationType opType, uint32_t result, uint32_t op2);

};
