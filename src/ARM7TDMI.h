#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#define NDEBUG 1;
//#define NDEBUGWARN 1;

#ifdef NDEBUG
#define DEBUG(x)
#else
#define DEBUG(x)        \
    do {                \
        std::cout << "INFO: " << x; \
    } while (0)
#endif


#ifdef NDEBUGWARN
#define DEBUGWARN(x)
#else
#define DEBUGWARN(x)        \
    do {                \
        std::cout << "WARN: " << x; \
    } while (0)
#endif

class Bus;

class ARM7TDMI {

   public:
    ARM7TDMI();
    ~ARM7TDMI();

    void initializeWithRom();
    uint32_t step();

    void clock();

    // CPU exceptions
    void irq();
    void firq();
    void reset();

    // dependency injection
    void connectBus(Bus *bus);

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
        uint8_t C : 1;  // C - Carry Flag      (0=Borrow/No Carry, 1=Carry/No Borrow
        uint8_t Z : 1;  // Z - Zero Flag       (0=Not Zero, 1=Zero)
        uint8_t N : 1;  // N - Sign Flag       (0=Not Signed, 1=Signed)
    };

    // struct representing the number of cycles an operation will take
    struct Cycles {
        uint8_t nonSequentialCycles : 8;
        uint8_t sequentialCycles : 8;
        uint8_t internalCycles : 8;
        uint8_t waitState : 8;
    };

    enum Interrupt : uint16_t {
        VBlank         = 0b1,
        HBlank         = 0b10,
        VCounterMatch  = 0b100,
        Timer0Overflow = 0b1000,
        Timer1Overflow = 0b10000,
        Timer2Overflow = 0b100000,
        Timer3Overflow = 0b1000000,
        SerialComm     = 0b10000000,
        DMA0           = 0b100000000,
        DMA1           = 0b1000000000,
        DMA2           = 0b10000000000,
        DMA3           = 0b100000000000,
        Keypad         = 0b1000000000000,
        GamePak        = 0b10000000000000
    };

    void queueInterrupt(Interrupt interrupt);

    // returns the SPSR for the CPU's current mode
    ProgramStatusRegister *getCurrentModeSpsr();

    ProgramStatusRegister getCpsr();

    // accounts for modes, ex in IRQ mode, getting register 14 will return value
    // of R14_irq
    uint32_t getRegister(uint8_t index);
    uint32_t getUserRegister(uint8_t index);

    void setRegister(uint8_t index, uint32_t value);

    void setCurrInstruction(uint32_t instruction);

    uint32_t getCurrentInstruction();
    Cycles getCurrentCycles();
    static uint32_t psrToInt(ProgramStatusRegister psr);

    ProgramStatusRegister cpsr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_fiq = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_svc = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_abt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_irq = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ProgramStatusRegister SPSR_und = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    bool debug = false;
  
   private:

    bool interruptsEnabled();

    enum FetchPCMemoryAccess {
        NONSEQUENTIAL,
        SEQUENTIAL,
        BRANCH
    };

    FetchPCMemoryAccess currentPcAccessType;

    uint32_t currInstruction; 

    void getNextInstruction(FetchPCMemoryAccess currentPcAccessType);

    class ArmOpcodeHandlers {
       public:
        static ARM7TDMI::FetchPCMemoryAccess multiplyHandler(uint32_t instruction,
                                                ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess dataProcHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess psrHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess undefinedOpHandler(uint32_t instruction,
                                                   ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess singleDataTransHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess halfWordDataTransHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess singleDataSwapHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess blockDataTransHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess branchHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess branchAndExchangeHandler(uint32_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess swiHandler(uint32_t instruction, ARM7TDMI *cpu);

    };

    class ThumbOpcodeHandlers {
       public:
        static ARM7TDMI::FetchPCMemoryAccess shiftHandler(uint16_t instruction,
                                             ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess addSubHandler(uint16_t instruction,
                                              ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess undefinedOpHandler(uint16_t instruction,
                                                   ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess immHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess aluHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess bxHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess loadPcRelativeHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess loadStoreRegOffsetHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess loadStoreSignExtendedByteHalfwordHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess loadStoreImmediateOffsetHandler(uint16_t instruction, ARM7TDMI *cpu); 
        static ARM7TDMI::FetchPCMemoryAccess loadStoreHalfwordHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess loadStoreSpRelativeHandler(uint16_t instruction, ARM7TDMI *cpu);      
        static ARM7TDMI::FetchPCMemoryAccess getRelativeAddressHandler(uint16_t instruction, ARM7TDMI *cpu); 
        static ARM7TDMI::FetchPCMemoryAccess addOffsetToSpHandler(uint16_t instruction, ARM7TDMI *cpu); 
        static ARM7TDMI::FetchPCMemoryAccess multipleLoadStorePushPopHandler(uint16_t instruction, ARM7TDMI *cpu);  
        static ARM7TDMI::FetchPCMemoryAccess multipleLoadStoreHandler(uint16_t instruction, ARM7TDMI *cpu);
        static ARM7TDMI::FetchPCMemoryAccess conditionalBranchHandler(uint16_t instruction, ARM7TDMI *cpu);     
        static ARM7TDMI::FetchPCMemoryAccess unconditionalBranchHandler(uint16_t instruction, ARM7TDMI *cpu);         
        static ARM7TDMI::FetchPCMemoryAccess longBranchHandler(uint16_t instruction, ARM7TDMI *cpu); 
        static ARM7TDMI::FetchPCMemoryAccess softwareInterruptHandler(uint16_t instruction, ARM7TDMI *cpu); 
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

    std::array<uint32_t *, 16> userRegisters = {
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
    uint32_t r8_fiq = 0x0;
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
    // TODO: temporary boot location for testing
    static const uint32_t BOOT_LOCATION = 0x08000000;
    //static const uint32_t BOOT_LOCATION = 0x00000000;

    uint8_t overflowBit = 0;
    uint8_t carryBit = 0;
    uint8_t zeroBit = 0;
    uint8_t signBit = 0;


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

    static uint8_t thumbGetRs(uint16_t instruction);
    static uint8_t thumbGetRd(uint16_t instruction);
    static uint8_t thumbGetRb(uint16_t instruction);
        
    static bool dataTransGetP(uint32_t instruction);
    static bool dataTransGetU(uint32_t instruction);
    static bool dataTransGetB(uint32_t instruction);
    static bool dataTransGetW(uint32_t instruction);
    static bool dataTransGetL(uint32_t instruction);

    static bool sFlagSet(uint32_t instruction);

    static uint8_t mulGetExecutionTimeMVal(uint32_t value);
    static uint8_t umullGetExecutionTimeMVal(uint32_t value);

    
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



    enum Condition {
        EQ = 0x0, // 0:   EQ     Z=1           equal (zero) (same)
        NE = 0x1, // 1:   NE     Z=0           not equal (nonzero) (not same)
        CS = 0x2, // 2:   CS/HS  C=1           unsigned higher or same (carry set)
        CC = 0x3, // 3:   CC/LO  C=0           unsigned lower (carry cleared)
        MI = 0x4, // 4:   MI     N=1           signed negative (minus)
        PL = 0x5, // 5:   PL     N=0           signed positive or zero (plus)
        VS = 0x6, // 6:   VS     V=1           signed overflow (V set)
        VC = 0x7, // 7:   VC     V=0           signed no overflow (V cleared)
        HI = 0x8, // 8:   HI     C=1 and Z=0   unsigned higher
        LS = 0x9, // 9:   LS     C=0 or Z=1    unsigned lower or same
        GE = 0xA, // A:   GE     N=V           signed greater or equal
        LT = 0xB, // B:   LT     N<>V          signed less than
        GT = 0xC, // C:   GT     Z=0 and N=V   signed greater than
        LE = 0xD, // D:   LE     Z=1 or N<>V   signed less or equal
        AL = 0xE, // E:   AL     -             always (the "AL" suffix can be omitted)
        NV = 0xF  // F:   NV     -             never (ARMv1,v2 only) (Reserved ARMv3 and up)
    };

    // shifts the second operand according to ALU logic. returns the shifted
    // operand and the carry bit
    AluShiftResult aluShift(uint32_t instruction, bool i, bool r);

    typedef FetchPCMemoryAccess (*ArmOpcodeHandler)(uint32_t, ARM7TDMI *);
    typedef FetchPCMemoryAccess (*ThumbOpcodeHandler)(uint16_t, ARM7TDMI *);

    ArmOpcodeHandler decodeArmInstruction(uint32_t instruction);
    ThumbOpcodeHandler decodeThumbInstruction(uint16_t instruction);

    bool conditionalHolds(uint8_t cond);

    Cycles executeInstruction(uint32_t rawInstruction);

    // accounts for modes, ex in IRQ mode, setting register 14 will set value of
    // R14_irq
    void setUserRegister(uint8_t index, uint32_t value);


    void switchToMode(Mode mode);

};
