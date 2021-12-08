
#include "ARM7TDMI.h"

#include <bit>
#include <bitset>
#include <iostream>
#include <type_traits>
#include <string.h>

#include "Bus.h"
#include "assert.h"
#include "Timer.h"

#include "ArmOpcodeHandlers.cpp"
#include "ThumbOpcodeHandlers.cpp"

ARM7TDMI::ARM7TDMI() {
}

void ARM7TDMI::initializeWithRom() {
    switchToMode(SYSTEM);
    cpsr.T = 0; // set CPU to ARM state
    cpsr.Z = 1; // why? TODO: find out
    cpsr.C = 1;
    setRegister(PC_REGISTER, BOOT_LOCATION); 
    currentPcAccessType = BRANCH;
    // TODO: find out why setting register 0 and 1
    setRegister(0, 0x08000000);
    setRegister(1, 0x000000EA); 
    setRegister(SP_REGISTER, 0x03007F00); // stack pointer
    r13_svc = 0x03007FE0; // SP_svc=03007FE0h
    r13_irq = 0x03007FA0; // SP_irq=03007FA0h

    bus->resetCycleCountTimeline();
    uint32_t pcAddress = getRegister(PC_REGISTER);
    currInstruction = bus->read32(pcAddress, Bus::CycleType::NONSEQUENTIAL);
    DEBUG(std::bitset<32>(currInstruction).to_string() << " <- boot location instruction \n");
    // emulate filling the pipeline
    bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 4, 32);
    bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 8, 32);


}

ARM7TDMI::~ARM7TDMI() {}

uint32_t ARM7TDMI::getCurrentInstruction() {
    return currInstruction;
}


uint32_t ARM7TDMI::step() {
    //DEBUG((uint32_t)cpsr.Mode << " <- current mode\n");
    // TODO: give this method a better name
    bus->resetCycleCountTimeline();
    //DEBUGWARN("cpsr.i: " << (uint32_t)cpsr.I << "\n");
   // DEBUGWARN("start\n");

    if((bus->iORegisters[Bus::IORegister::IME] & 0x1) && 
        (!cpsr.I) &&
       ((bus->iORegisters[Bus::IORegister::IE] & bus->iORegisters[Bus::IORegister::IF]) || 
       ((bus->iORegisters[Bus::IORegister::IE + 1] & 0x3F) & (bus->iORegisters[Bus::IORegister::IF + 1] & 0x3F)))) {
        // interrupts is enabled
        //DEBUGWARN("irq fn start\n");
        //DEBUGWARN("IF lo: " << (uint32_t)(bus->iORegisters[Bus::IORegister::IF]) << "\n");
        //DEBUGWARN("IF hi: " << (uint32_t)(bus->iORegisters[Bus::IORegister::IF + 1]) << "\n");
        irq();
        //DEBUGWARN("irq fn ended\n");

    }

    if (!cpsr.T) {  // check state bit, is CPU in ARM state?

        uint8_t cond = (currInstruction & 0xF0000000) >> 28;
        DEBUG("in arm state\n");

        setRegister(PC_REGISTER, getRegister(PC_REGISTER) + 4);

        if(conditionalHolds(cond)) {
            currentPcAccessType = executeArmInstruction(currInstruction);
        } else {
            currentPcAccessType = SEQUENTIAL;
        }
        // increment PC

    } else {  // THUMB state
        //DEBUG("in thumb state. Going to execute thumb instruction " << std::bitset<16>(currInstruction).to_string() << "\n");
        setRegister(PC_REGISTER, getRegister(PC_REGISTER) + 2);
        currentPcAccessType = executeThumbInstruction(currInstruction);
    }


    getNextInstruction(currentPcAccessType);

    //DEBUGWARN("end\n");

    // TODO: just return one cycle per instr for now
    return 2;
}

inline
void ARM7TDMI::getNextInstruction(FetchPCMemoryAccess currentPcAccessType) {
    DEBUG("getting instruction from: " << getRegister(PC_REGISTER) << "\n");
    currInstrAddress = getRegister(PC_REGISTER);
    switch(currentPcAccessType) {
        case SEQUENTIAL: {
            currInstruction = 
                cpsr.T ? bus->read16(currInstrAddress, Bus::CycleType::SEQUENTIAL) :
                         bus->read32(currInstrAddress, Bus::CycleType::SEQUENTIAL);
            break;
        }
        case NONSEQUENTIAL: {
            currInstruction = 
                cpsr.T ? bus->read16(currInstrAddress, Bus::CycleType::NONSEQUENTIAL) :
                         bus->read32(currInstrAddress, Bus::CycleType::NONSEQUENTIAL);                
            break;
        }
        case BRANCH: {
            if(cpsr.T) {
                currInstruction = bus->read16(currInstrAddress, Bus::CycleType::NONSEQUENTIAL);
                DEBUG(std::bitset<16>(currInstruction).to_string() << " <- instruction after branching \n");
                // emulate filling the pipeline
                //bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 2, 16);
                //bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 4, 16);
            } else {
                currInstruction = bus->read32(currInstrAddress, Bus::CycleType::NONSEQUENTIAL);
                DEBUG(std::bitset<32>(currInstruction).to_string() << " <- instruction after branching \n");
                // emulate filling the pipeline
                //bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 4, 32);
                //bus->addCycleToExecutionTimeline(Bus::CycleType::SEQUENTIAL, pcAddress + 8, 32);
            }
            break;
        }
        case NONE: {
            break;
        }
    }
}

inline
void ARM7TDMI::irq() {
    //DEBUGWARN("irq1\n");

    uint32_t returnAddr = getRegister(PC_REGISTER) + 4;
    
    switchToMode(Mode::IRQ);
    // switch to ARM mode
    cpsr.T = 0;
    cpsr.I = 1; 
    setRegister(PC_REGISTER, 0x18);
    setRegister(LINK_REGISTER, returnAddr);
    getNextInstruction(FetchPCMemoryAccess::BRANCH);
    //DEBUGWARN("irq2\n");
}

void ARM7TDMI::queueInterrupt(Interrupt interrupt) {
    bus->iORegisters[Bus::IORegister::IF] |= ((uint16_t)interrupt & 0xFF);
    bus->iORegisters[Bus::IORegister::IF + 1] |= (((uint16_t)interrupt >> 8) & 0xFF);
}

void ARM7TDMI::connectBus(Bus *bus) { 
    this->bus = bus; 
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
            assert(!(bool)(value & 0x00000020));
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
    |_Cond__|0_0_0_1_0|Op_|0|Rd/RdHi|Rn/RdLo|__Rs___|1|y|x|0|__Rm___|
MulHalfARM9
    |_Cond__|0_0_0|P|U|0|W|L|__Rn___|__Rd___|0_0_0_0|1|S|H|1|__Rm___| TransReg10
    |_Cond__|0_0_0|P|U|1|W|L|__Rn___|__Rd___|OffsetH|1|S|H|1|OffsetL| TransImm10
    |_Cond__|0_1_0|P|U|B|W|L|__Rn___|__Rd___|_________Offset________| TransImm9
    |_Cond__|0_1_1|P|U|B|W|L|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| TransReg9
    |_Cond__|0_1_1|________________xxx____________________|1|__xxx__| Undefined
    |_Cond__|1_0_0|P|U|S|W|L|__Rn___|__________Register_List________| BlockTrans
    |_Cond__|1_0_1|L|___________________Offset______________________| B,BL,BLX
    |_Cond__|1_1_1_1|_____________Ignored_by_Processor______________| SWI

decoding from highest to lowest specifity to ensure corredct opcode parsed

    case: 000 (bit 27, 26, 25)

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

    case 010:

        xx: xxxx010xxxxxxxxxxxxxxxxxxxxxxxxx    transImm9

    case 011:

        11: xxxx011xxxxxxxxxxxxxxxxxxxx0xxxx    TransReg9   (4)
        13: xxxx011xxxxxxxxxxxxxxxxxxxx1xxxx    Undefined   (4)

*/
// TODO: use hex values to make it more syntactically concise
inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::executeArmInstruction(uint32_t instruction) {

    switch (instruction & 0b00001110000000000000000000000000) {  // mask 1
        case 0b00000000000000000000000000000000: {

            if ((instruction & 0b00001111111111111111111111010000) == 0b00000001001011111111111100010000) {  // BX,BLX
                return branchAndExchangeHandler(instruction);

            } else if ((instruction & 0b00001111101100000000111111110000) == 0b00000001000000000000000010010000) {  // TransSwp12
                return singleDataSwapHandler(instruction);

            } else if ((instruction & 0b00001111100100000000111111110000) == 0b00000001000000000000000000000000) {  // PSR Reg
                return psrHandler(instruction);

            } else if ((instruction & 0b00001111110000000000000011110000) == 0b00000000000000000000000010010000) {  // Multiply
                return multiplyHandler(instruction);

            } else if ((instruction & 0b00001110010000000000111110010000) == 0b00000000000000000000000010010000) {  // TransReg10
                return halfWordDataTransHandler(instruction);

            } else if ((instruction & 0b00001111100000000000000011110000) == 0b00000000100000000000000010010000) {  // MulLong
                return multiplyHandler(instruction);

            } else if ((instruction & 0b00001110010000000000000010010000) == 0b00000000010000000000000010010000) {  // TransImm10
                return halfWordDataTransHandler(instruction);

            } else {  // dataProc
                // debugger->disassembleDataProcessing(instruction);
                return dataProcHandler(instruction);
            }
            break;
        }
        case 0b00000010000000000000000000000000: {
            if ((instruction & 0b00001111101100000000000000000000) == 0b00000011001000000000000000000000) {  // PSR Imm
                return psrHandler(instruction);

            } else {  // DataProc
                // debugger->disassembleDataProcessing(instruction);
                return dataProcHandler(instruction);
            }
        }
        case 0b00001000000000000000000000000000: { // block transfer
            return blockDataTransHandler(instruction);

        }
        case 0b00001010000000000000000000000000: { // B,BL,BLX
            return branchHandler(instruction);

        }
        case 0b00001110000000000000000000000000: { // SWI
            // TODO: implement software interrupt
            //DEBUGWARN("ARM SWI!!!\n");
            return swiHandler(instruction);
        }
        case 0b00000100000000000000000000000000: {  // transImm9
            return singleDataTransHandler(instruction);

        }
        case 0b00000110000000000000000000000000: {
            if ((instruction & 0b00001110000000000000000000010000) == 0b00000110000000000000000000000000) {  // TransReg9
                return singleDataTransHandler(instruction);

            } else {  // Undefined
                return undefinedOpHandler(instruction);
            }
            break;
        }
        default: {
            return undefinedOpHandler(instruction);
        }
    }
    return undefinedOpHandler(instruction);
}

/*


 Form|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
 __1_|_0___0___0_|__Op___|_______Offset______|____Rs_____|____Rd_____|Shifted
 __2_|_0___0___0___1___1_|_I,_Op_|___Rn/nn___|____Rs_____|____Rd_____|ADD/SUB
 __3_|_0___0___1_|__Op___|____Rd_____|_____________Offset____________|Immedi.
 __4_|_0___1___0___0___0___0_|______Op_______|____Rs_____|____Rd_____|AluOp
 __5_|_0___1___0___0___0___1_|__Op___|Hd_|Hs_|____Rs_____|____Rd_____|HiReg/BX
 __6_|_0___1___0___0___1_|____Rd_____|_____________Word______________|LDR PC
 __7_|_0___1___0___1_|__Op___|_0_|___Ro______|____Rb_____|____Rd_____|LDR/STR
 __8_|_0___1___0___1_|__Op___|_1_|___Ro______|____Rb_____|____Rd_____|""H/SB/SH
 __9_|_0___1___1_|__Op___|_______Offset______|____Rb_____|____Rd_____|""{B}
 _10_|_1___0___0___0_|Op_|_______Offset______|____Rb_____|____Rd_____|""H
 _11_|_1___0___0___1_|Op_|____Rd_____|_____________Word______________|"" SP
 _12_|_1___0___1___0_|Op_|____Rd_____|_____________Word______________|ADD PC/SP
 _13_|_1___0___1___1___0___0___0___0_|_S_|___________Word____________|ADD SP,nn
 _14_|_1___0___1___1_|Op_|_1___0_|_R_|____________Rlist______________|PUSH/POP
 _15_|_1___1___0___0_|Op_|____Rb_____|____________Rlist______________|STM/LDM
 _16_|_1___1___0___1_|_____Cond______|_________Signed_Offset_________|B{cond}
 _17_|_1___1___0___1___1___1___1___1_|___________User_Data___________|SWI
 _18_|_1___1___1___0___0_|________________Offset_____________________|B
 _19_|_1___1___1___1_|_H_|______________Offset_Low/High______________|BL,BLX

 decoding from highest to lowest specifity to ensure correct opcode parsed

    case 000:
        2: 00011xxxxxxxxxxx ADD/SUB
        1: 000xxxxxxxxxxxxx Shifted
    case 001:
        3: 001xxxxxxxxxxxxx Immedi.
    case 010:
        4: 010000xxxxxxxxxx AluOp
`       5: 010001xxxxxxxxxx HiReg/BX
        6: 01001xxxxxxxxxxx LDR PC

        7: 0101xx0xxxxxxxxx LDR/STR
        8: 0101xx1xxxxxxxxx ""H/SB/SH
    case 011:
        9: 011xxxxxxxxxxxxx ""{B}
    case 100:
       10: 1000xxxxxxxxxxxx "H
       11: 1001xxxxxxxxxxxx "" SP
    case 101: 
       13: 10110000xxxxxxxx ADD SP,nn
       14: 1011x10xxxxxxxxx PUSH/POP

       12: 1010xxxxxxxxxxxx ADD PC/SP
    case 110: 
       17: 11011111xxxxxxxx SWI

       15: 1100xxxxxxxxxxxx STM/LDM
       16: 1101xxxxxxxxxxxx B{cond}
    case 111:
       18: 11100xxxxxxxxxxx B
       19: 1111xxxxxxxxxxxx BL,BLX
*/
inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::executeThumbInstruction(uint16_t instruction) {
    switch (instruction & 0b1110000000000000) {  // mask 1
        case 0b0000000000000000: { // case 000
            if((instruction & 0b0001100000000000) == 0b0001100000000000) {
                // 2: 00011xxxxxxxxxxx ADD/SUB
                return addSubHandler(instruction);
            } else {
                // 1: 000xxxxxxxxxxxxx Shifted
                return shiftHandler(instruction);
            }
        }
        case 0b0010000000000000: { // case 001
            // 3: 001xxxxxxxxxxxxx Immedi.
            return immHandler(instruction);
        }
        case 0b0100000000000000: { // case 010
            switch(instruction & 0b0001000000000000) {
                case 0b0000000000000000: {
                    switch(instruction & 0b0001110000000000) {
                        case 0b0000000000000000: {
                            // 4: 010000xxxxxxxxxx AluOp     
                            return aluHandler(instruction);              
                        }
                        case 0b0000010000000000: {
                            // 5: 010001xxxxxxxxxx HiReg/BX
                            return bxHandler(instruction);
                        }
                        default: {
                            // 6: 01001xxxxxxxxxxx LDR PC
                            return loadPcRelativeHandler(instruction);
                        }
                    }
                }
                case 0b0001000000000000: {
                    if(instruction & 0b0000001000000000) {
                        // 8: 0101xx1xxxxxxxxx ""H/SB/SH
                        return loadStoreSignExtendedByteHalfwordHandler(instruction);
                    } else {
                        // 7: 0101xx0xxxxxxxxx LDR/STR
                        return loadStoreRegOffsetHandler(instruction);
                    }
                }
                default: {
                    break;
                }                
            }
            break;
        }
        case 0b0110000000000000: { // case 011
            // 9: 011xxxxxxxxxxxxx ""{B}
            return loadStoreImmediateOffsetHandler(instruction);
        }
        case 0b1000000000000000: { // case 100
            if(instruction & 0b0001000000000000) {
                // 11: 1001xxxxxxxxxxxx "" SP
                return loadStoreSpRelativeHandler(instruction);
            } else {
                // 10: 1000xxxxxxxxxxxx "H
                return loadStoreHalfwordHandler(instruction);
            }
            break;
        }
        case 0b1010000000000000: { // case 101
            if(instruction & 0b0001000000000000) {
                if(instruction & 0b0000010000000000) {
                    // 14: 1011x10xxxxxxxxx PUSH/POP
                    return multipleLoadStorePushPopHandler(instruction);
                } else {
                    // 13: 10110000xxxxxxxx ADD SP,nn
                    return addOffsetToSpHandler(instruction);
                }
            } else {
                // 12: 1010xxxxxxxxxxxx ADD PC/SP
                return getRelativeAddressHandler(instruction);
            }
        }
        case 0b1100000000000000: { // case 110
            if(instruction & 0b0001000000000000) {
                if((instruction & 0b0001111100000000) == 0b0001111100000000) {
                    // 17: 11011111xxxxxxxx SWI
                    return softwareInterruptHandler(instruction);
                } else {
                    // 16: 1101xxxxxxxxxxxx B{cond}
                    return conditionalBranchHandler(instruction);
                }
            } else {
                // 15: 1100xxxxxxxxxxxx STM/LDM
                return multipleLoadStoreHandler(instruction);
            }           
        }
        case 0b1110000000000000: { // case 111
            if(instruction & 0b0001000000000000) {
                // 19: 1111xxxxxxxxxxxx BL,BLX
                return longBranchHandler(instruction);
            } else {
                // 18: 11100xxxxxxxxxxx B
                return unconditionalBranchHandler(instruction);
            }
            break;
        }        
        default: {
            break;
        }
    }
    // undefined opcode
    return undefinedOpHandler(instruction);
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

        DEBUG("shifting immediate\n");
        uint32_t imm = instruction & 0x000000FF;
        uint8_t is = (instruction & 0x00000F00) >> 7U;
        uint32_t op2 = aluShiftRor(imm, is % 32);

        DEBUG(std::bitset<32>(imm).to_string() << " before shift \n");
        DEBUG(std::bitset<32>(op2).to_string() << " after shift \n");

        DEBUG(imm << " is imm\n");
        DEBUG((uint32_t)is << " is shift amount\n");

        // carry out bit is the least significant discarded bit of rm
        if (is > 0) {
            carryBit = (imm >> (is - 1)) & 0x1;
        } else {
            carryBit = cpsr.C;
        }
        DEBUG((uint32_t)carryBit << " is carryBit\n");
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
    DEBUG((uint32_t)shiftType << " is shift type \n");
    DEBUG((uint32_t)shiftAmount << " is shiftAmount \n");
    DEBUG(r << " = r \n");
    DEBUG(immOpIsZero << " = immOpIsZero \n");

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
            DEBUG(std::bitset<32>(rm).to_string() << " before shift \n");
            DEBUG(std::bitset<32>(op2).to_string() << " after shift \n");
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
            DEBUG(std::bitset<32>(rm).to_string() << " before shift \n");
            DEBUG(std::bitset<32>(op2).to_string() << " after shift \n");
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

// TODO: separate these utility functions into a different file
// TODO: make sure all these shift functions are correct
uint32_t ARM7TDMI::aluShiftLsl(uint32_t value, uint8_t shift) {
    if(shift >= 32) {
        return 0;
    } else {
        return value << shift;
    }
}

uint32_t ARM7TDMI::aluShiftLsr(uint32_t value, uint8_t shift) {
    if(shift >= 32) {
        return 0;
    } else {
        return value >> shift;
    }
}

uint32_t ARM7TDMI::aluShiftAsr(uint32_t value, uint8_t shift) {
    if(shift >= 32) {
        return (value & 0x80000000) ? 0xFFFFFFFF : 0x0;
    } else {
       return (value & 0x80000000) ? ~(~value >> shift) : value >> shift; 
    }
}

uint32_t ARM7TDMI::aluShiftRor(uint32_t value, uint8_t shift) {
    assert(shift < 32U);
    return (value >> shift) | (value << (-((int8_t)shift) & 31U));
}

uint32_t ARM7TDMI::aluShiftRrx(uint32_t value, uint8_t shift, ARM7TDMI *cpu) {
    assert(shift < 32U);
    uint32_t rrxMask = (cpu->cpsr).C;
    DEBUG(rrxMask << " <- rrx mask\n");
    return ((value >> shift) | (value << (-shift & 31U))) | (rrxMask << 31U);
}

ARM7TDMI::ProgramStatusRegister *ARM7TDMI::getCurrentModeSpsr() {
    return currentSpsr;
}

ARM7TDMI::ProgramStatusRegister ARM7TDMI::getCpsr() {
    return cpsr;
}

uint32_t ARM7TDMI::getRegister(uint8_t index) { return *(registers[index]); }

uint32_t ARM7TDMI::getUserRegister(uint8_t index) { return *(userRegisters[index]); }

void ARM7TDMI::setRegister(uint8_t index, uint32_t value) {
    *(registers[index]) = value;
}
void ARM7TDMI::setUserRegister(uint8_t index, uint32_t value) {
    *(userRegisters[index]) = value;
}

bool ARM7TDMI::aluSetsZeroBit(uint32_t value) { return value == 0; }

bool ARM7TDMI::aluSetsSignBit(uint32_t value) { return value >> 31; }

bool ARM7TDMI::aluSubtractSetsCarryBit(uint32_t rnValue, uint32_t op2) {
    return !(rnValue < op2);
}

bool ARM7TDMI::aluSubtractSetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                          uint32_t result) {
    // todo: there is a more efficient way to do this
    return (!(rnValue & 0x80000000) && (op2 & 0x80000000) &&
            (result & 0x80000000)) ||
           ((rnValue & 0x80000000) && !(op2 & 0x80000000) &&
            !(result & 0x80000000));
}

bool ARM7TDMI::aluAddSetsCarryBit(uint32_t rnValue, uint32_t op2) {
    return (0xFFFFFFFFU - op2) < rnValue;
}

bool ARM7TDMI::aluAddSetsOverflowBit(uint32_t rnValue, uint32_t op2,
                                     uint32_t result) {
    // todo: there is a more efficient way to do this
    return ((rnValue & 0x80000000) && (op2 & 0x80000000) &&
            !(result & 0x80000000)) ||
            (!(rnValue & 0x80000000) && !(op2 & 0x80000000) &&
            (result & 0x80000000));
}

bool ARM7TDMI::aluAddWithCarrySetsCarryBit(uint64_t result) {
    return result >> 32;
}

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

bool ARM7TDMI::aluSubWithCarrySetsCarryBit(uint64_t result) {
    return !(result >> 32);
}

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

uint8_t ARM7TDMI::thumbGetRs(uint16_t instruction) {
    return (instruction & 0x0038) >> 3;
}

uint8_t ARM7TDMI::thumbGetRd(uint16_t instruction) {
    return (instruction & 0x0007);
}

uint8_t ARM7TDMI::thumbGetRb(uint16_t instruction) {
    return (instruction & 0x0038) >> 3;
}


uint8_t ARM7TDMI::getOpcode(uint32_t instruction) {
    return (instruction & 0x01E00000) >> 21;
}

bool ARM7TDMI::sFlagSet(uint32_t instruction) {
    return (instruction & 0x00100000);
}

uint32_t ARM7TDMI::psrToInt(ProgramStatusRegister psr) {
    return 0 | (((uint32_t)psr.N) << 31) | (((uint32_t)psr.Z) << 30) |
           (((uint32_t)psr.C) << 29) | (((uint32_t)psr.V) << 28) |
           (((uint32_t)psr.Q) << 27) | (((uint32_t)psr.Reserved) << 26) |
           (((uint32_t)psr.I) << 7) | (((uint32_t)psr.F) << 6) |
           (((uint32_t)psr.T) << 5) | (((uint32_t)psr.Mode) << 0);
}

bool ARM7TDMI::dataTransGetP(uint32_t instruction) {
    return instruction & 0x01000000;
}

bool ARM7TDMI::dataTransGetU(uint32_t instruction) {
    return instruction & 0x00800000;
}

bool ARM7TDMI::dataTransGetB(uint32_t instruction) {
    return instruction & 0x00400000;
}

bool ARM7TDMI::dataTransGetW(uint32_t instruction) {
    return instruction & 0x00200000;
}

bool ARM7TDMI::dataTransGetL(uint32_t instruction) {
    return instruction & 0x00100000;
}


/*
Execution Time: 1S+mI for MUL, and 1S+(m+1)I for MLA. 
Whereas 'm' depends on whether/how many most significant 
bits of Rs are all zero or all one. That is m=1 for 
Bit 31-8, m=2 for Bit 31-16, m=3 for Bit 31-24, and m=4 otherwise.
*/
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
*/
// uint32_t ARM7TDMI::psrToInt(ProgramStatusRegister psr) {
//     uint32_t value = 0;

//     value |= ((bool)psr.N << 31);
//     value |= ((bool)psr.Z << 30);
//     value |= ((bool)psr.C << 29);
//     value |= ((bool)psr.V << 28);
//     value |= ((bool)psr.Q << 27);
//     value |= ((bool)psr.I << 7);
//     value |= ((bool)psr.F << 6);
//     value |= ((bool)psr.T << 5);
//     value |= (psr.Mode & 0x1F);

//     return value;
// }