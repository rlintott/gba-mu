#include "../../memory/Bus.h"



template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armBranchHandler(uint32_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0x0E000000) == 0x0A000000);

    int32_t offset = ((instruction & 0x00FFFFFF) << 2);
    if(offset & 0x02000000) {
        // negative? Then must sign extend
        offset |= 0xFE000000;
    }

    BitPreservedInt32 pcVal;
    pcVal._unsigned = cpu->getRegister(PC_REGISTER);
    BitPreservedInt32 branchAddr;

    // TODO might be able to remove +4 when after implekemting pipelining
    branchAddr._signed = pcVal._signed + offset + 4;
    branchAddr._unsigned &= 0xFFFFFFFC;

    if constexpr(!(op & 0x100)) {
        // B
    } else {
        // BL LR=PC+4
        cpu->setRegister(LINK_REGISTER, cpu->getRegister(PC_REGISTER));
    }

    cpu->setRegister(PC_REGISTER, branchAddr._unsigned);
    return BRANCH;
}