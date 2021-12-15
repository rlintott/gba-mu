#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"



template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armBranchXHandler(uint32_t instruction, ARM7TDMI* cpu) {
    assert(((instruction & 0x0FFFFF00) >> 8) == 0b00010010111111111111);
    constexpr uint8_t opcode = op & 0x00F;

    // for this op rn is where rm usually is
    uint8_t rn = cpu->getRm(instruction);
    assert(rn != PC_REGISTER);
    uint32_t rnVal = cpu->getRegister(rn);
    
    if constexpr(opcode == 0x1) {
        // // BX PC=Rn, T=Rn.0
    } else if constexpr(opcode == 0x3) {
        // BLX PC=Rn, T=Rn.0, LR=PC+4
        cpu->setRegister(LINK_REGISTER, cpu->getRegister(PC_REGISTER));
    } else {
        // ?
    }

    /*
        For ARM code, the low bits of the target address should be usually zero,
        otherwise, R15 is forcibly aligned by clearing the lower two bits.
        For THUMB code, the low bit of the target address may/should/must be
        set, the bit is (or is not) interpreted as thumb-bit (depending on the
        opcode), and R15 is then forcibly aligned by clearing the lower bit. In
        short, R15 will be always forcibly aligned, so mis-aligned branches won't
        have effect on subsequent opcodes that use R15, or [R15+disp] as operand.
    */
    bool t = rnVal & 0x1;
    cpu->cpsr.T = t;
    if (t) {
        rnVal &= 0xFFFFFFFE;
    } else {
        rnVal &= 0xFFFFFFFC;
    }
    cpu->setRegister(PC_REGISTER, rnVal);

    return BRANCH;

}