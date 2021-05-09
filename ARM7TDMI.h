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
        0x1,
        0x2,
        0x3,
        0x4,
        0x5,
        0x6,
        0x7,
        0x8,
        0x9,
        0xA,
        0xB,
        0xC,
        0xD,
        0xE,    // stack pointer
        0xF,    // link register
        0x0     // program counter
    };

    uint32_t r8_fiq = 69;
    uint32_t r9_fiq = 0x0;
    uint32_t r10_fiq = 0x0;
    uint32_t r11_fiq = 0x0;
    uint32_t r12_fiq = 0x0;
    uint32_t r13_fiq = 0x0;
    uint32_t r14_fiq = 0x0;

    //  8 - 14 "banked" registers for mode FIQ
    std::array<uint32_t*, 16> fiqRegisters = {
        registers.data() + (0),
        registers.data() + (1),
        registers.data() + (2),
        registers.data() + (3),
        registers.data() + (4),
        registers.data() + (5),
        registers.data() + (6),
        registers.data() + (7),
        &r8_fiq,
        &r9_fiq,
        &r10_fiq,
        &r11_fiq,
        &r12_fiq,
        &r13_fiq,
        &r14_fiq,
        registers.data() + (15)
    };

    uint32_t r13_irq = 0x0;
    uint32_t r14_irq = 0x0;

    //  13 - 14 "banked" registers for mode IRQ
    std::array<uint32_t*, 16> irqRegisters = {
        registers.data() + (0),
        registers.data() + (1),
        registers.data() + (2),
        registers.data() + (3),
        registers.data() + (4),
        registers.data() + (5),
        registers.data() + (6),
        registers.data() + (7),
        registers.data() + (8),
        registers.data() + (9),
        registers.data() + (10),
        registers.data() + (11),
        registers.data() + (12),
        &r13_fiq,
        &r14_fiq,
        registers.data() + (15)
    };

    uint32_t r13_svc = 0x0;
    uint32_t r14_svc = 0x0;

    //  13 - 14 "banked" registers for mode SVC
    std::array<uint32_t*, 16> svcRegisters = {
        registers.data() + (0),
        registers.data() + (1),
        registers.data() + (2),
        registers.data() + (3),
        registers.data() + (4),
        registers.data() + (5),
        registers.data() + (6),
        registers.data() + (7),
        registers.data() + (8),
        registers.data() + (9),
        registers.data() + (10),
        registers.data() + (11),
        registers.data() + (12),
        &r13_svc,
        &r14_svc,
        registers.data() + (15)
    };

    uint32_t r13_abt = 0x0;
    uint32_t r14_abt = 0x0;

    //  13 - 14 "banked" registers for mode ABT
     std::array<uint32_t*, 16> abtRegisters = {
        registers.data() + (0),
        registers.data() + (1),
        registers.data() + (2),
        registers.data() + (3),
        registers.data() + (4),
        registers.data() + (5),
        registers.data() + (6),
        registers.data() + (7),
        registers.data() + (8),
        registers.data() + (9),
        registers.data() + (10),
        registers.data() + (11),
        registers.data() + (12),
        &r13_abt,
        &r14_abt,
        registers.data() + (15)
    };

    uint32_t r13_und = 0x0;
    uint32_t r14_und = 0x0;

    //  13 - 14 "banked" registers for mode UND
     std::array<uint32_t*, 16> undRegisters = {
        registers.data() + (0),
        registers.data() + (1),
        registers.data() + (2),
        registers.data() + (3),
        registers.data() + (4),
        registers.data() + (5),
        registers.data() + (6),
        registers.data() + (7),
        registers.data() + (8),
        registers.data() + (9),
        registers.data() + (10),
        registers.data() + (11),
        registers.data() + (12),
        &r13_und,
        &r14_und,
        registers.data() + (15)
    };

    // 
    static const uint8_t PC_REGISTER = 15; 
    static const uint8_t LINK_REGISTER = 14; 
    static const uint8_t SP_REGISTER = 13; 

    // struct representing program status register (xPSR)
    struct ProgramStatusRegister {
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

    struct AluShiftResult {
        uint32_t op2;
        uint8_t carry;
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

    ProgramStatusRegister cpsr = {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_fiq = {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_svc = {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_abt = {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_irq = {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_und = {0,0,0,0,0,0,0,0,0,0};


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

    // shifts the second operand according to ALU logic. returns the shifted operand and the carry bit
    AluShiftResult aluShift(uint32_t instruction, bool i, bool r);

    void aluUpdateCpsrFlags(AluOperationType opType, uint32_t result, uint32_t op2, uint8_t cycles);

    // returns the SPSR for the CPU's current mode
    ProgramStatusRegister getModeSpsr();

};
