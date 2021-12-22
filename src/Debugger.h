
#include <cstdint>
#include <array>
#include <string>
#include <capstone.h>

class ARM7TDMI;
class Bus;

// TODO: very quick and rough debugger. In the future make it more sophisticated and make a UI 
class Debugger {

    public: 
        Debugger();

        void updateState(ARM7TDMI* cpu, Bus* bus);
        void printState();
        void step(ARM7TDMI* cpu, Bus* bus);
        static bool stepMode;
        std::string disassembleArm(uint32_t instruction);

    private:

        csh sCapstone;
        
        uint32_t watchAddr1 = 0x04000208;
        uint8_t watchAddr1Width = 32;

        uint32_t watchAddr2 = 0x04000200;
        uint8_t watchAddr2Width = 8;

        uint32_t watchAddr3 = 0x04000202;
        uint8_t watchAddr3Width = 8;

        uint32_t watchAddr4 = 0x00000000;
        uint8_t watchAddr4Width = 8;

        uint32_t watchAddr5 = 0x00000000;
        uint8_t watchAddr5Width = 8;

        uint32_t watchAddr6 = 0x00000000;
        uint8_t watchAddr6Width = 8;

        uint32_t r0; 
        uint32_t r1; 
        uint32_t r2; 
        uint32_t r3; 
        uint32_t r4; 
        uint32_t r5; 
        uint32_t r6; 
        uint32_t r7;
        uint32_t r8; 
        uint32_t r9; 
        uint32_t r10;
        uint32_t r11;
        uint32_t r12;
        uint32_t r13;
        uint32_t r14;
        uint32_t r15;
        uint32_t r13irq;
        uint32_t r14irq;
        uint32_t r13svc;
        uint32_t r14svc;
        uint32_t r13und;
        uint32_t r14und;
        uint32_t watchMem1; 
        uint32_t cpsr;
        uint32_t watchMem2;
        uint32_t spsr_svc; 
        uint32_t watchMem3;
        uint32_t spsr_fiq;
        uint32_t watchMem4;
        uint32_t spsr_abt;
        uint32_t watchMem5;
        uint32_t spsr_irq;
        uint32_t watchMem6;
        uint32_t spsr_und; 
        uint32_t instrAddress; 
        std::string instr;
        uint32_t instrWord;

        uint32_t breakpoint = 0x08000B84;
};