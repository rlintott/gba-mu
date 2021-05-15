#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#ifdef NDEBUG
#define DEBUG(x)
#else
#define DEBUG(x)        \
    do {                \
        std::cerr << x; \
    } while (0)
#endif

class Bus;

class ARM7TDMI {
   public:
    ARM7TDMI();
    ~ARM7TDMI();

    // struct representing the number of cycles an operation will take
   private:
    struct Cycles {
        uint8_t nonSequentialCycles : 8;
        uint8_t sequentialCycles : 8;
        uint8_t internalCycles : 8;
        uint8_t waitState : 8;
    };

    class ArmOpcodeHandlers {
       public:
        static ARM7TDMI::Cycles multiplyHandler(uint32_t instruction,
                                                ARM7TDMI *cpu);
        static ARM7TDMI::Cycles aluHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::Cycles psrHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::Cycles undefinedOpHandler(uint32_t instruction,
                                                   ARM7TDMI *cpu);
        static ARM7TDMI::Cycles sdtHandler(uint32_t instruction, ARM7TDMI *cpu);
    };

    union BitPreservedInt32 {
        int32_t _signed;
        uint32_t _unsigned;
    };

    union BitPreservedInt64 {
        int64_t _signed;
        uint64_t _unsigned;
    };

    // registers can be dynamically changed to support different registers for
    // different CPU modes
    std::array<uint32_t *, 16> registers = {
        &r0,  &r1, &r2, &r3, &r4, &r5, &r6, &r7, &r8, &r9, &r10, &r11, &r12,
        &r13,  // stack pointer
        &r14,  // link register
        &r15   // program counter
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
        uint8_t Mode : 5;  //  M4-M0 - Mode Bits
        uint8_t T : 1;  // T - State Bit       (0=ARM, 1=THUMB) - Do not change
                        // manually!
        uint8_t F : 1;  // F - FIQ disable     (0=Enable, 1=Disable)
        uint8_t I : 1;  // I - IRQ disable     (0=Enable, 1=Disable)
        uint32_t Reserved : 19;  // Reserved            (For future use) - Do
                                 // not change manually!
        uint8_t Q : 1;  // Q - Sticky Overflow (1=Sticky Overflow, ARMv5TE and
                        // up only)
        uint8_t V : 1;  // V - Overflow Flag   (0=No Overflow, 1=Overflow)
        uint8_t
            C : 1;  // C - Carry Flag      (0=Borrow/No Carry, 1=Carry/No Borrow
        uint8_t Z : 1;  // Z - Zero Flag       (0=Not Zero, 1=Zero)
        uint8_t N : 1;  // N - Sign Flag       (0=Not Signed, 1=Signed)
    };

    // todo: deprecate in favour of shifting op2 in place and only returning
    // carry
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

    ProgramStatusRegister cpsr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_fiq = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_svc = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_abt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_irq = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_und = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    ProgramStatusRegister *currentSpsr;

    Bus *bus;

    Cycles UNDEF(uint32_t instruction);

    static uint32_t aluShiftLsl(uint32_t value, uint8_t shift);
    static uint32_t aluShiftLsr(uint32_t value, uint8_t shift);
    static uint32_t aluShiftAsr(uint32_t value, uint8_t shift);
    static uint32_t aluShiftRor(uint32_t value, uint8_t shift);
    static uint32_t aluShiftRrx(uint32_t value, uint8_t shift, ARM7TDMI *cpu);

    Cycles execAluOpcode(uint8_t opcode, uint32_t rd, uint32_t op1,
                         uint32_t op2);

    static bool aluSetsZeroBit(uint32_t value);
    static bool aluSetsSignBit(uint32_t value);
    static bool aluSubtractSetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                           uint32_t result);
    static bool aluSubtractSetsCarryBit(uint32_t rnValue, uint32_t op2);
    static bool aluAddSetsCarryBit(uint32_t rnValue, uint32_t op2);
    static bool aluAddSetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                      uint32_t result);
    static bool aluAddWithCarrySetsCarryBit(uint64_t result);
    static bool aluAddWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                               uint32_t result, ARM7TDMI *cpu);
    static bool aluSubWithCarrySetsCarryBit(uint64_t result);
    static bool aluSubWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                               uint32_t result, ARM7TDMI *cpu);

    static uint8_t getRd(uint32_t instruction);
    static uint8_t getRn(uint32_t instruction);
    static uint8_t getRs(uint32_t instruction);
    static uint8_t getRm(uint32_t instruction);
    static uint8_t getOpcode(uint32_t instruction);

    static bool dataTransGetP(uint32_t instruction);
    static bool dataTransGetU(uint32_t instruction);
    static bool dataTransGetB(uint32_t instruction);
    static bool dataTransGetW(uint32_t instruction);
    static bool dataTransGetL(uint32_t instruction);

    static bool sFlagSet(uint32_t instruction);

    static uint32_t psrToInt(ProgramStatusRegister psr);
    void transferToPsr(uint32_t value, uint8_t field,
                       ProgramStatusRegister *psr);

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

    // shifts the second operand according to ALU logic. returns the shifted
    // operand and the carry bit
    AluShiftResult aluShift(uint32_t instruction, bool i, bool r);

    typedef Cycles (*ArmOpcodeHandler)(uint32_t, ARM7TDMI *);

    typedef Cycles (*ThumbOpcodeHandler)(uint16_t);

    ArmOpcodeHandler decodeArmInstruction(uint32_t instruction);

   public:
    void step();

    void clock();

    void enterInstructionCycle();

    // CPU exceptions
    void irq();
    void firq();
    void undefinedInstruction();
    void reset();

    Cycles executeInstruction(uint32_t rawInstruction);

    // accounts for modes, ex in IRQ mode, getting register 14 will return value
    // of R14_irq
    uint32_t getRegister(uint8_t index);

    // accounts for modes, ex in IRQ mode, setting register 14 will set value of
    // R14_irq
    void setRegister(uint8_t index, uint32_t value);

    // returns the SPSR for the CPU's current mode
    ProgramStatusRegister *getCurrentModeSpsr();

    // dependency injection
    void connectBus(Bus *bus);

    void switchToMode(Mode mode);

    friend class Disassembler;
};
