#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbSwiHandler(uint16_t instruction, ARM7TDMI* cpu) {
    // 11011111b: SWI nn   ;software interrupt
    // 10111110b: BKPT nn  ;software breakpoint (ARMv5 and up) not used in ARMv4T
    assert((instruction & 0xFF00) == 0xDF00);

    // cpu->cpsr=<changed>  ;Enter svc/abt
    cpu->switchToMode(Mode::SUPERVISOR);

    // ARM state, IRQs disabled
    cpu->cpsr.T = 0;
    cpu->cpsr.I = 1;

    // R14_svc=PC+2    ;save return address
    // TODO explain why you arent adding 2
    cpu->setRegister(14, cpu->getRegister(PC_REGISTER));

    // PC=VVVV0008h    ;jump to SWI/PrefetchAbort vector address
    // TODO: is base always 0000?
    // TODO: make vector addresses static members
    cpu->setRegister(PC_REGISTER, 0x00000008);

    return BRANCH; 
}
