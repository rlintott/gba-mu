#include "Debugger.h"
#include "ARM7TDMI.h"
#include "Bus.h"


#define WORD_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define WORD_TO_BINARY(word)  \
  (word & 0x80000000 ? '1' : '0'), \
  (word & 0x40000000 ? '1' : '0'), \
  (word & 0x20000000 ? '1' : '0'), \
  (word & 0x10000000 ? '1' : '0'), \
  (word & 0x08000000 ? '1' : '0'), \
  (word & 0x04000000 ? '1' : '0'), \
  (word & 0x02000000 ? '1' : '0'), \
  (word & 0x01000000 ? '1' : '0'), \
  (word & 0x00800000 ? '1' : '0'), \
  (word & 0x00400000 ? '1' : '0'), \
  (word & 0x00200000 ? '1' : '0'), \
  (word & 0x00100000 ? '1' : '0'), \
  (word & 0x00080000 ? '1' : '0'), \
  (word & 0x00040000 ? '1' : '0'), \
  (word & 0x00020000 ? '1' : '0'), \
  (word & 0x00010000 ? '1' : '0'), \
  (word & 0x00008000 ? '1' : '0'), \
  (word & 0x00004000 ? '1' : '0'), \
  (word & 0x00002000 ? '1' : '0'), \
  (word & 0x00001000 ? '1' : '0'), \
  (word & 0x00000800 ? '1' : '0'), \
  (word & 0x00000400 ? '1' : '0'), \
  (word & 0x00000200 ? '1' : '0'), \
  (word & 0x00000100 ? '1' : '0'), \
  (word & 0x00000080 ? '1' : '0'), \
  (word & 0x00000040 ? '1' : '0'), \
  (word & 0x00000020 ? '1' : '0'), \
  (word & 0x00000010 ? '1' : '0'), \
  (word & 0x00000008 ? '1' : '0'), \
  (word & 0x00000004 ? '1' : '0'), \
  (word & 0x00000002 ? '1' : '0'), \
  (word & 0x00000001 ? '1' : '0')

Debugger::Debugger() {
    if (cs_open(CS_ARCH_ARM, CS_MODE_ARM, &sCapstone) != CS_ERR_OK) {
        puts("cs_open failed");
        exit(1);
    }
    cs_option(sCapstone, CS_OPT_DETAIL, CS_OPT_ON);
}


void Debugger::step(ARM7TDMI* cpu, Bus* bus) {
    if(stepMode) {
        updateState(cpu, bus);
    } else if(cpu->currInstrAddress == breakpoint) {
        printf("Hit breakpoint, press LShift to step though instructions\n");
        stepMode = true;
        updateState(cpu, bus);
    } else {  
    }
}


void Debugger::printState() {
    printf(
        "\
**********************************CPU STATE***********************************\n\
r0:     0x%08X      r1:     0x%08X      r2:     0x%08X      r3:     0x%08X    \n\
r4:     0x%08X      r5:     0x%08X      r6:     0x%08X      r7:     0x%08X    \n\
r8:     0x%08X      r9:     0x%08X      r10:    0x%08X      r11:    0x%08X    \n\
r12:    0x%08X      r13(SP):0x%08X      r14(LR):0x%08X      r15(PC):0x%08X    \n\
\n\
r13irq: 0x%08X      r14irq: 0x%08X\n\
r13svc: 0x%08X      r14svc: 0x%08X\n\
r13und: 0x%08X      r14und: 0x%08X\n\
\n\
                                                  WATCH MEMORY:\n\
flags:      NZCV------------------EAIFTMODE       [0xFFE34185](32):   0x%08XA\n\
cpsr:       " WORD_TO_BINARY_PATTERN "      [0xFFE34185](8):    0x%08X\n\
spsr_svc:   " WORD_TO_BINARY_PATTERN "      [0xFFE34185](8):    0x%08X\n\
spsr_fiq:   " WORD_TO_BINARY_PATTERN "      [0xFFE34185](16):   0x%08X\n\
spsr_abt:   " WORD_TO_BINARY_PATTERN "      [0xFFE34185](8):    0x%08X\n\
spsr_irq:   " WORD_TO_BINARY_PATTERN "      [0xFFE34185](8):    0x%08X\n\
spsr_und:   " WORD_TO_BINARY_PATTERN "\n\
\n\
[0x%08X]: %s    (0x%08X)\n\
******************************************************************************\n",
      r0, r1, r2, r3, 
      r4, r5, r6, r7,
      r8, r9, r10,r11,
      r12,r13,r14,r15,
      r13irq,r14irq,
      r13svc,r14svc,
      r13und,r14und,
      watchMem1, 
      WORD_TO_BINARY(cpsr),
      watchMem2,
      WORD_TO_BINARY(spsr_svc), 
      watchMem3,
      WORD_TO_BINARY(spsr_fiq),
      watchMem4,
      WORD_TO_BINARY(spsr_abt),
      watchMem5,
      WORD_TO_BINARY(spsr_irq),
      watchMem6,
      WORD_TO_BINARY(spsr_und), 
      instrAddress, instr.c_str(), instrWord);
}



void Debugger::updateState(ARM7TDMI* cpu, Bus* bus) {
    // r0 = cpu->getRegisterDebug(0);
    // r1 = cpu->getRegisterDebug(1);
    // r2 = cpu->getRegisterDebug(2);
    // r3 = cpu->getRegisterDebug(3);
    // r4 = cpu->getRegisterDebug(4);
    // r5 = cpu->getRegisterDebug(5);
    // r6 = cpu->getRegisterDebug(6);
    // r7 = cpu->getRegisterDebug(7);
    // r8 = cpu->getRegisterDebug(8);
    // r9 = cpu->getRegisterDebug(9);
    // r10= cpu->getRegisterDebug(10);
    // r11= cpu->getRegisterDebug(11);
    // r12= cpu->getRegisterDebug(12);
    // r13= cpu->getRegisterDebug(13);
    // r14= cpu->getRegisterDebug(14);
    // r15= cpu->getRegisterDebug(15);
    // r13irq = cpu->r13_irq;
    // r14irq = cpu->r14_irq;
    // r13svc = cpu->r13_svc;
    // r14svc = cpu->r14_svc;
    // r13und = cpu->r13_und;
    // r14und = cpu->r14_und;
    // cpsr = ARM7TDMI::psrToInt(cpu->cpsr);
    // spsr_abt = ARM7TDMI::psrToInt(cpu->SPSR_abt);
    // spsr_svc = ARM7TDMI::psrToInt(cpu->SPSR_svc);
    // spsr_irq = ARM7TDMI::psrToInt(cpu->SPSR_irq);
    // spsr_fiq = ARM7TDMI::psrToInt(cpu->SPSR_fiq);
    // spsr_und = ARM7TDMI::psrToInt(cpu->SPSR_und);

    // instrAddress = cpu->currInstrAddress;
    // instr = disassembleArm(cpu->currInstruction);
    // instrWord = cpu->currInstruction;
}

static uint32_t signExtend23Bit(uint32_t value) {
    return (value & 0x00400000) ? (value | 0xFF800000) : value;
}

std::string Debugger::disassembleArm(uint32_t instruction) {
    struct cs_insn *insn;
    char buffer[50];
    bool thumbMode = cpsr & 0x20;
    cs_option(sCapstone, CS_OPT_MODE, ((thumbMode) ? CS_MODE_THUMB : CS_MODE_ARM) | CS_MODE_LITTLE_ENDIAN);
    uint32_t count;
    if(thumbMode) {
        // in THUMB MODE
        uint8_t instr[2];
        instr[0] = (instruction) & 0xFF;
        instr[1] = (instruction >> 8) & 0xFF;
        count = cs_disasm(sCapstone, instr, 2, 0, 1, &insn);
    } else {
        uint8_t instr[4];
        instr[0] = (instruction) & 0xFF;
        instr[1] = (instruction >> 8) & 0xFF;
        instr[2] = (instruction >> 16) & 0xFF;
        instr[3] = (instruction >> 24) & 0xFF;
        count = cs_disasm(sCapstone, instr, 4, 0, 1, &insn);
    }  
    if(count == 1) {
        sprintf(buffer, "%s %s", insn[0].mnemonic, insn[0].op_str); 
    } else {
        if((instruction & 0xF000) == 0xF000 && thumbMode) {
            // bl instruction isnt disassembled by capstone...
            bool lo = instruction & 0x0800;
            uint32_t offset = lo ? ((instruction & 0x07FF)) << 1 : signExtend23Bit((instruction & 0x07FF) << 12);

            sprintf(buffer, "%s #0x%08X", (lo ? "bl lo" : "bl hi"), offset);
        } else {
            DEBUGWARN("capstone disassemble count != 1, instead is " << count << " \n");
        }

    }    
    cs_free(insn, count);

    return buffer;
}