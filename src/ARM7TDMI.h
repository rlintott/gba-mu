#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "util/static_for.h"
#include "assert.h"

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

    
    static uint32_t aluShiftRor(uint32_t value, uint8_t shift);
    static uint32_t aluShiftRrx(uint32_t value, uint8_t shift, ARM7TDMI *cpu);
  
   private:
    

    bool interruptsEnabled();

    enum FetchPCMemoryAccess {
        NONSEQUENTIAL,
        SEQUENTIAL,
        BRANCH,
        NONE
    };

    FetchPCMemoryAccess currentPcAccessType;

    uint32_t currInstruction;
    uint32_t currInstrAddress; 

    void getNextInstruction(FetchPCMemoryAccess currentPcAccessType);

    FetchPCMemoryAccess multiplyHandler(uint32_t instruction);
    FetchPCMemoryAccess dataProcHandler(uint32_t instruction);
    FetchPCMemoryAccess psrHandler(uint32_t instruction);
    FetchPCMemoryAccess undefinedOpHandler(uint32_t instruction);
    FetchPCMemoryAccess singleDataTransHandler(uint32_t instruction);
    FetchPCMemoryAccess halfWordDataTransHandler(uint32_t instruction);
    FetchPCMemoryAccess singleDataSwapHandler(uint32_t instruction);
    FetchPCMemoryAccess blockDataTransHandler(uint32_t instruction);
    FetchPCMemoryAccess branchHandler(uint32_t instruction);
    FetchPCMemoryAccess branchAndExchangeHandler(uint32_t instruction);
    FetchPCMemoryAccess swiHandler(uint32_t instruction);

    FetchPCMemoryAccess shiftHandler(uint16_t instruction);
    FetchPCMemoryAccess addSubHandler(uint16_t instruction);
    FetchPCMemoryAccess undefinedOpHandler(uint16_t instruction);
    FetchPCMemoryAccess immHandler(uint16_t instruction);
    FetchPCMemoryAccess aluHandler(uint16_t instruction);
    FetchPCMemoryAccess bxHandler(uint16_t instruction);
    FetchPCMemoryAccess loadPcRelativeHandler(uint16_t instruction);
    FetchPCMemoryAccess loadStoreRegOffsetHandler(uint16_t instruction);
    FetchPCMemoryAccess loadStoreSignExtendedByteHalfwordHandler(uint16_t instruction);
    FetchPCMemoryAccess loadStoreImmediateOffsetHandler(uint16_t instruction); 
    FetchPCMemoryAccess loadStoreHalfwordHandler(uint16_t instruction);
    FetchPCMemoryAccess loadStoreSpRelativeHandler(uint16_t instruction);      
    FetchPCMemoryAccess getRelativeAddressHandler(uint16_t instruction); 
    FetchPCMemoryAccess addOffsetToSpHandler(uint16_t instruction); 
    FetchPCMemoryAccess multipleLoadStorePushPopHandler(uint16_t instruction);  
    FetchPCMemoryAccess multipleLoadStoreHandler(uint16_t instruction);
    FetchPCMemoryAccess conditionalBranchHandler(uint16_t instruction);     
    FetchPCMemoryAccess unconditionalBranchHandler(uint16_t instruction);         
    FetchPCMemoryAccess longBranchHandler(uint16_t instruction); 
    FetchPCMemoryAccess softwareInterruptHandler(uint16_t instruction);

    uint32_t thumbLongbranchShift = 0;

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

    static constexpr uint8_t PC_REGISTER = 15;
    static constexpr uint8_t LINK_REGISTER = 14;
    static constexpr uint8_t SP_REGISTER = 13;
    // TODO: temporary boot location for testing
    static constexpr uint32_t BOOT_LOCATION = 0x08000000;
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

    Cycles execAluOpcode(uint8_t opcode, uint32_t rd, uint32_t op1,
                         uint32_t op2);

    static uint32_t aluShiftLsl(uint32_t value, uint8_t shift);
    static uint32_t aluShiftLsr(uint32_t value, uint8_t shift);
    static uint32_t aluShiftAsr(uint32_t value, uint8_t shift);

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

    FetchPCMemoryAccess executeArmInstruction(uint32_t instruction);
    FetchPCMemoryAccess executeThumbInstruction(uint16_t instruction);

    bool conditionalHolds(uint8_t cond);

    Cycles executeInstruction(uint32_t rawInstruction);

    // accounts for modes, ex in IRQ mode, setting register 14 will set value of
    // R14_irq
    void setUserRegister(uint8_t index, uint32_t value);


    void switchToMode(Mode mode);

    // TODO: temporary
    friend class Debugger;

    using ArmHandler = FetchPCMemoryAccess (*)(uint32_t instruction, ARM7TDMI* cpu);

    template<uint16_t op> static FetchPCMemoryAccess armDataProcHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armPsrHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armMultHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armStdHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armUndefHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armHwdtHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armSdsHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armBdtHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armSdtHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armBranchHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armBranchXHandler(uint32_t instruction, ARM7TDMI* cpu);
    template<uint16_t op> static FetchPCMemoryAccess armSwiHandler(uint32_t instruction, ARM7TDMI* cpu);

    static constexpr std::array<ArmHandler, 4096> armLut = [] {
        std::array<ArmHandler, 4096> lut = {};

        /*
        BX:
            0001 0010 00x1 (11)
        Swap:
            0001 0x00 1001 (11)
        PSR:
            0001 0xx0 0000 (10)
        Mult:
            0000 00xx 1001 (10)
        MulLong:
            0000 1xxx 1001 (9)
        PSR:
            0011 0x10 xxxx (7)
        SDT:
            (halfword)
            000x x0xx 1xx1 (6)
            000x x1xx 1xx1 (6)
        Dataproc:
            000x xxxx 0xx1 (5)
            000x xxxx xxx0 (4)
            001x xxxx xxxx (4)
        Undefined:
            011x xxxx xxx1 (4)
        SWI:
            1111 xxxx xxxx (4)
        SDT:
            (single)
            011x xxxx xxx0 (4)
            010x xxxx xxxx (3)
        BlockTrans:
            100x xxxx xxxx (3)
        Branch:
            101x xxxx xxxx (3)
        */
        auto decodeHandler = [&] (auto i) {
            switch((i & 0xE00) >> 9) {
                case 0b000: {
                    if((i & 0b000111111101) == 0b000100100001) {
                        return &armBranchXHandler<i.value>;
                    } else if((i & 0b000110111111) == 0b000100001001) {
                        return &armSdsHandler<i.value>;
                    } else if((i & 0b000110011111) == 0b000100000000) {
                        return &armPsrHandler<i.value>;
                    } else if((i & 0b000111001111) == 0b000000001001) {
                        return &armMultHandler<i.value>;
                    } else if((i & 0b000110001111) == 0b000010001001) {
                        return &armMultHandler<i.value>;
                    } else if((i & 0b000000001001) == 0b000000001001) {
                        return &armHwdtHandler<i.value>;
                    } else if((i & 0b000000001001) == 0b000000000001) {
                        return &armDataProcHandler<i.value>;
                    } else if((i & 0b000000000001) == 0b000000000000) {
                        return &armDataProcHandler<i.value>;
                    } else {
                    }
                    break;
                }
                case 0b001: {
                    if((i & 0b000110110000) == 0b000100100000) {
                        return &armPsrHandler<i.value>;
                    } else { 
                        return &armDataProcHandler<i.value>;
                    }                   
                    break;
                } 
                case 0b011: {
                    if(i & 0x1) {
                        return &armUndefHandler<i.value>;
                    } else {
                        return &armSdtHandler<i.value>;
                    }
                    break;
                }
                case 0b111: {
                    return &armSwiHandler<i.value>;
                }
                case 0b010: {
                    return &armSdtHandler<i.value>;
                }
                case 0b100: {
                    return &armBdtHandler<i.value>;
                }
                case 0b101: {
                    return &armBranchHandler<i.value>;
                }
                default: {
                    return &armUndefHandler<i.value>;
                }
            }
            return &armUndefHandler<i.value>;
        };
        /* 
            Since using recursive template metaprogramming, we have to
            build the LUT in chunks of 500 to avoid
            compiler stack overflow....
        */
        static_for<0, 500>::apply([&](auto i)
        {     
            lut[i] = decodeHandler(i);    
        });
        static_for<500, 1000>::apply([&](auto i)
        {   
            lut[i] = decodeHandler(i);           
        });
        static_for<1000, 1500>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        static_for<1500, 2000>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        static_for<2000, 2500>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        static_for<2500, 3000>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        static_for<3000, 3500>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        static_for<3500, 4000>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        static_for<4000, 4096>::apply([&](auto i)
        {            
            lut[i] = decodeHandler(i);
        });
        return lut;
    }();
};


constexpr std::array<ARM7TDMI::ArmHandler, 4096> ARM7TDMI::armLut;


inline
uint32_t ARM7TDMI::aluShiftLsl(uint32_t value, uint8_t shift) {
    if(shift >= 32) {
        return 0;
    } else {
        return value << shift;
    }
}

inline
uint32_t ARM7TDMI::aluShiftLsr(uint32_t value, uint8_t shift) {
    if(shift >= 32) {
        return 0;
    } else {
        return value >> shift;
    }
}

inline
uint32_t ARM7TDMI::aluShiftAsr(uint32_t value, uint8_t shift) {
    if(shift >= 32) {
        return (value & 0x80000000) ? 0xFFFFFFFF : 0x0;
    } else {
       return (value & 0x80000000) ? ~(~value >> shift) : value >> shift; 
    }
}


inline
uint32_t ARM7TDMI::aluShiftRor(uint32_t value, uint8_t shift) {
    //assert(shift < 32U);
    return (value >> shift) | (value << (-((int8_t)shift) & 31U));
}


inline
bool ARM7TDMI::aluSetsZeroBit(uint32_t value) { return value == 0; }

inline
bool ARM7TDMI::aluSetsSignBit(uint32_t value) { return value >> 31; }
inline
bool ARM7TDMI::aluSubtractSetsCarryBit(uint32_t rnValue, uint32_t op2) {
    return !(rnValue < op2);
}
inline
bool ARM7TDMI::aluSubtractSetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                          uint32_t result) {
    // todo: there is a more efficient way to do this
    return (!(rnValue & 0x80000000) && (op2 & 0x80000000) &&
            (result & 0x80000000)) ||
           ((rnValue & 0x80000000) && !(op2 & 0x80000000) &&
            !(result & 0x80000000));
}
inline
bool ARM7TDMI::aluAddSetsCarryBit(uint32_t rnValue, uint32_t op2) {
    return (0xFFFFFFFFU - op2) < rnValue;
}
inline
bool ARM7TDMI::aluAddSetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                     uint32_t result) {
    // todo: there is a more efficient way to do this
    return ((rnValue & 0x80000000) && (op2 & 0x80000000) &&
            !(result & 0x80000000)) ||
            (!(rnValue & 0x80000000) && !(op2 & 0x80000000) &&
            (result & 0x80000000));
}
inline
bool ARM7TDMI::aluAddWithCarrySetsCarryBit(uint64_t result) {
    return result >> 32;
}
inline
bool ARM7TDMI::aluAddWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                              uint32_t result, ARM7TDMI *cpu) {
    // todo: there is a more efficient way to do this
    return ((rnValue & 0x80000000) && (op2 & 0x80000000) &&
            !((rnValue + op2) & 0x80000000)) ||
           (!(rnValue & 0x80000000) && !(op2 & 0x80000000) &&
            ((rnValue + op2) & 0x80000000)) ||
           // ((rnValue + op2) & 0x80000000) && (cpsr.C & 0x80000000) &&
           // !(((uint32_t)result) & 0x80000000)) ||  never happens
           (!((rnValue + op2) & 0x80000000) && !(cpu->cpsr.C & 0x80000000) &&
            ((result)&0x80000000));
}
inline
bool ARM7TDMI::aluSubWithCarrySetsCarryBit(uint64_t result) {
    return !(result >> 32);
}
inline
bool ARM7TDMI::aluSubWithCarrySetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                              uint32_t result, ARM7TDMI *cpu) {
    // todo: there is a more efficient way to do this
    return ((rnValue & 0x80000000) && ((~op2) & 0x80000000) &&
            !((rnValue + (~op2)) & 0x80000000)) ||
           (!(rnValue & 0x80000000) && !((~op2) & 0x80000000) &&
            ((rnValue + (~op2)) & 0x80000000)) ||
           // (((rnValue + (~op2)) & 0x80000000) && (cpsr.C & 0x80000000) &&
           // !(result & 0x80000000)) || never happens
           (!((rnValue + (~op2)) & 0x80000000) && !(cpu->cpsr.C & 0x80000000) &&
            (result & 0x80000000));
}

inline
uint32_t ARM7TDMI::psrToInt(ProgramStatusRegister psr) {
    return 0 | (((uint32_t)psr.N) << 31) | (((uint32_t)psr.Z) << 30) |
           (((uint32_t)psr.C) << 29) | (((uint32_t)psr.V) << 28) |
           (((uint32_t)psr.Q) << 27) | (((uint32_t)psr.Reserved) << 26) |
           (((uint32_t)psr.I) << 7) | (((uint32_t)psr.F) << 6) |
           (((uint32_t)psr.T) << 5) | (((uint32_t)psr.Mode) << 0);
}

inline
bool ARM7TDMI::dataTransGetP(uint32_t instruction) {
    return instruction & 0x01000000;
}

inline
bool ARM7TDMI::dataTransGetU(uint32_t instruction) {
    return instruction & 0x00800000;
}

inline
bool ARM7TDMI::dataTransGetB(uint32_t instruction) {
    return instruction & 0x00400000;
}

inline
bool ARM7TDMI::dataTransGetW(uint32_t instruction) {
    return instruction & 0x00200000;
}

inline
bool ARM7TDMI::dataTransGetL(uint32_t instruction) {
    return instruction & 0x00100000;
}

inline
void ARM7TDMI::switchToMode(Mode mode) {
    //DEBUGWARN((uint32_t)mode << " <- switching to mode\n");
    switch (mode) {
        case SYSTEM:
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
            // DEBUGWARN("supervisor mode\n");
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
    }
    // SPSR_svc=CPSR   ;save CPSR flags
    *(getCurrentModeSpsr()) = cpsr;
    cpsr.Mode = mode;
}

inline
void ARM7TDMI::transferToPsr(uint32_t value, uint8_t field,
                             ProgramStatusRegister *psr) {
    if (field & 0b1000) {
        // TODO: is this correct? it says   f =  write to flags field     Bit
        // 31-24 (aka _flg)
        psr->N = (bool)(value & 0x80000000);
        psr->Z = (bool)(value & 0x40000000);
        psr->C = (bool)(value & 0x20000000);
        psr->V = (bool)(value & 0x10000000);
        psr->Q = (bool)(value & 0x08000000);
        psr->Reserved = (psr->Reserved & 0b0001111111111111111) | ((value & 0x07000000) >> 8);
    }
    if (field & 0b0100) {
        // reserved, don't change
    }
    if (field & 0b0010) {
        // reserverd don't change
    }
    if (cpsr.Mode != USER) {
        if (field & 0b0001) {
            psr->I = (bool)(value & 0x00000080);
            psr->F = (bool)(value & 0x00000040);
            // t bit may not be changed, for THUMB/ARM switching use BX
            // instruction.
            //assert(!(bool)(value & 0x00000020));
            psr->T = (bool)(value & 0x00000020);
            uint8_t mode = (value & 0x00000010) | (value & 0x00000008) |
                           (value & 0x00000004) | (value & 0x00000002) |
                           (value & 0x00000001);
            psr->Mode = mode;
            
            // TODO: implemnt less hacky way ti transfer psr
            if(psr == &cpsr) {
                //DEBUGWARN("in transfer to PSR changing mode\n");
                switchToMode(ARM7TDMI::Mode(mode));
            }   
        }
    }
}



/*
Execution Time: 1S+mI for MUL, and 1S+(m+1)I for MLA. 
Whereas 'm' depends on whether/how many most significant 
bits of Rs are all zero or all one. That is m=1 for 
Bit 31-8, m=2 for Bit 31-16, m=3 for Bit 31-24, and m=4 otherwise.
*/
inline
uint8_t ARM7TDMI::mulGetExecutionTimeMVal(uint32_t value) {
    uint32_t masked = value & 0xFFFFFF00;
    if(masked == 0xFFFFFF00 || !masked) {
        return 1;
    }
    masked >>= 8;
    if(masked == 0xFFFFFF00 || !masked) {
        return 2;
    }
    masked >>= 8;
    if(masked == 0xFFFFFF00 || !masked) {
        return 3;
    }
    return 4;
}

inline
uint8_t ARM7TDMI::umullGetExecutionTimeMVal(uint32_t value) {
    uint32_t masked = value & 0xFFFFFF00;
    if(!masked) {
        return 1;
    }
    masked >>= 8;
    if(!masked) {
        return 2;
    }
    masked >>= 8;
    if(!masked) {
        return 3;
    }
    return 4;
}


// not guaranteed to always be rn, check the spec first
inline
uint8_t ARM7TDMI::getRn(uint32_t instruction) {
    return (instruction & 0x000F0000) >> 16;
}

// not guaranteed to always be rd, check the spec first
inline
uint8_t ARM7TDMI::getRd(uint32_t instruction) {
    return (instruction & 0x0000F000) >> 12;
}

// not guaranteed to always be rs, check the spec first
inline
uint8_t ARM7TDMI::getRs(uint32_t instruction) {
    return (instruction & 0x00000F00) >> 8;
}

inline
uint8_t ARM7TDMI::getRm(uint32_t instruction) {
    return (instruction & 0x0000000F);
}

inline
uint32_t ARM7TDMI::aluShiftRrx(uint32_t value, uint8_t shift, ARM7TDMI *cpu) {
    assert(shift < 32U);
    uint32_t rrxMask = (cpu->cpsr).C;
    DEBUG(rrxMask << " <- rrx mask\n");
    return ((value >> shift) | (value << (-shift & 31U))) | (rrxMask << 31U);
}
