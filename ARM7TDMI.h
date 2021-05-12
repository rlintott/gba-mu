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

    // registers can be dynamically changed to support different registers for different CPU modes 
    std::array<uint32_t*, 16> registers = {
        &r0,
        &r1,
        &r2,
        &r3,
        &r4,
        &r5,
        &r6,
        &r7,
        &r8,
        &r9,
        &r10,
        &r11,
        &r12,
        &r13,    // stack pointer
        &r14,    // link register
        &r15     // program counter
    };

    // all the possible registers
    uint32_t r0 = 0;
    uint32_t r1 = 0;
    uint32_t r2 = 0;
    uint32_t r3 = 0;
    uint32_t r4 = 0;
    uint32_t r5 = 0;
    uint32_t r6 = 0;
    uint32_t r7 = 0;
    uint32_t r8 = 0;
    uint32_t r9 = 0;
    uint32_t r10 = 0;
    uint32_t r11 = 0;
    uint32_t r12 = 0;
    uint32_t r13 = 0;
    uint32_t r14 = 0;
    uint32_t r15 = 0;
    uint32_t r8_fiq = 69;
    uint32_t r9_fiq = 0x0;
    uint32_t r10_fiq = 0x0;
    uint32_t r11_fiq = 0x0;
    uint32_t r12_fiq = 0x0;
    uint32_t r13_fiq = 0x0;
    uint32_t r14_fiq = 0x0;
    uint32_t r13_irq = 0x0;
    uint32_t r14_irq = 0x0;
    uint32_t r13_svc = 0x0;
    uint32_t r14_svc = 0x0;
    uint32_t r13_abt = 0x0;
    uint32_t r14_abt = 0x0;
    uint32_t r13_und = 0x0;
    uint32_t r14_und = 0x0;

    static const uint8_t PC_REGISTER = 15; 
    static const uint8_t LINK_REGISTER = 14; 
    static const uint8_t SP_REGISTER = 13; 
    static const uint32_t BOOT_LOCATION = 0x0;

    uint8_t overflowBit = 0;
    uint8_t carryBit = 0;
    uint8_t zeroBit = 0;
    uint8_t signBit = 0;

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
        uint8_t Z : 1;          // Z - Zero Flag       (0=Not Zero, 1=Zero)   
        uint8_t N : 1;          // N - Sign Flag       (0=Not Signed, 1=Signed)               
    };

    // todo: deprecate in favour of shifting op2 in place and only returning carry
    struct AluShiftResult {
        uint32_t op2;
        uint8_t carry;
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

    ProgramStatusRegister cpsr =        {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_fiq =    {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_svc =    {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_abt =    {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_irq =    {0,0,0,0,0,0,0,0,0,0};
    ProgramStatusRegister SPSR_und =    {0,0,0,0,0,0,0,0,0,0};

    ProgramStatusRegister* currentSpsr;

    // struct representing the number of cycles an operation will take
    struct Cycles {
        uint8_t nonSequentialCycles : 8;
        uint8_t sequentialCycles : 8;
        uint8_t internalCycles : 8;
        uint8_t waitState : 8;
    };

    Bus *bus;

    Cycles UNDEF(uint32_t instruction);

    uint32_t aluShiftLsl(uint32_t value, uint8_t shift);
    uint32_t aluShiftLsr(uint32_t value, uint8_t shift);
    uint32_t aluShiftAsr(uint32_t value, uint8_t shift);
    uint32_t aluShiftRor(uint32_t value, uint8_t shift);
    uint32_t aluShiftRrx(uint32_t value, uint8_t shift);

    Cycles execAluInstruction(uint32_t instruction);
    Cycles execAluOpcode(uint8_t opcode, uint32_t rd, uint32_t op1, uint32_t op2);
    Cycles execMultiplyInstruction(uint32_t instruction);
    Cycles execPsrTransferInstruction(uint32_t instruction);

    bool aluSetsZeroBit(uint32_t value);
    bool aluSetsSignBit(uint32_t value);
    bool aluSubtractSetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result);
    bool aluSubtractSetsCarryBit(uint32_t rnValue, uint32_t op2);
    bool aluAddSetsCarryBit(uint32_t rnValue, uint32_t op2);
    bool aluAddSetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result);
    bool aluAddWithCarrySetsCarryBit(uint64_t result);
    bool aluAddWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result);
    bool aluSubWithCarrySetsCarryBit(uint64_t result);
    bool aluSubWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2, uint32_t result);

    uint8_t getRd(uint32_t instruction);
    uint8_t getRn(uint32_t instruction);
    uint8_t getRs(uint32_t instruction);
    uint8_t getRm(uint32_t instruction);
    uint8_t getOpcode(uint32_t instruction);

    bool sFlagSet(uint32_t instruction);

    uint32_t psrToInt(ProgramStatusRegister psr);
    void transferToPsr(uint32_t value, uint8_t field, bool psrSource);

    enum AluOpcode {
        AND = 0x0,
        EOR = 0x1,
        SUB = 0x2,
        RSB = 0x3,
        ADD = 0x4,
        ADC = 0x5,
        SBC = 0x6,
        RSC = 0x7,
        TST = 0x8,
        TEQ = 0x9,
        CMP = 0xA,
        CMN = 0xB,
        ORR = 0xC,
        MOV = 0xD,
        BIC = 0xE,
        MVN = 0xF
    };


public:

    void step();

    void clock();

    // CPU exceptions
    void irq();
    void firq();
    void undefinedInstruction();
    void reset();

    Cycles executeInstruction(uint32_t rawInstruction);

    // accounts for modes, ex in IRQ mode, getting register 14 will return value of R14_irq
    uint32_t getRegister(uint8_t index);

    // accounts for modes, ex in IRQ mode, setting register 14 will set value of R14_irq
    void setRegister(uint8_t index, uint32_t value);

    // shifts the second operand according to ALU logic. returns the shifted operand and the carry bit
    AluShiftResult aluShift(uint32_t instruction, bool i, bool r);

    // returns the SPSR for the CPU's current mode
    ProgramStatusRegister* getCurrentModeSpsr();

    // dependency injection
    void connectBus(Bus* bus);

    void switchToMode(Mode mode);

};
