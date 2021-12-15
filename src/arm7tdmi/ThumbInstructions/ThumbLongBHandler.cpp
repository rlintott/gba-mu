#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbLongBHandler(uint16_t instruction, ARM7TDMI* cpu) {
    //uint8_t opcode = (instruction & 0xF800) >> 11;
    constexpr uint8_t opcode = (op & 0x3E0) >> 5;

    if constexpr(opcode == 0x1E) {
        // First Instruction - LR = PC+4+(nn SHL 12)
        assert((instruction & 0xF800) == 0xF000);
        uint32_t offsetHiBits = signExtend23Bit(((uint32_t)(instruction & 0x07FF)) << 12);            
        cpu->setRegister(LINK_REGISTER, cpu->getRegister(PC_REGISTER) + 2 + offsetHiBits);
        return SEQUENTIAL;
    } else if constexpr(opcode == 0x1F) {
        // Second Instruction - PC = LR + (nn SHL 1), and LR = PC+2 OR 1 (and BLX: T=0)
        // 11111b: BL label   ;branch long with link
        uint32_t offsetLoBits = ((uint32_t)(instruction & 0x07FF)) << 1;
        uint32_t temp = (cpu->getRegister(PC_REGISTER)) | 0x1;
        // The destination address range is (PC+4)-400000h..+3FFFFEh, 
        cpu->setRegister(PC_REGISTER, (cpu->getRegister(LINK_REGISTER) + offsetLoBits));
        cpu->setRegister(LINK_REGISTER, temp);  
        return BRANCH;
    } else if constexpr(opcode == 0x1D) {
        // 11101b: BLX label  ;branch long with link switch to ARM mode (ARM9)
        DEBUGWARN("BLX not implemented!\n");
        assert(false);
    } else {
        assert(false);
    }

    return SEQUENTIAL;
}
