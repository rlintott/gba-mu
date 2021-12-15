#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbRelAddrHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0xA000);
    //uint8_t opcode = (instruction & 0x0800) >> 11;
    constexpr bool opcode = (op & 0x020);
    //uint8_t rd = (instruction & 0x0700) >> 8;
    constexpr uint8_t rd = (op & 0x01C) >> 2;
    uint16_t offset = (instruction & 0x00FF) << 2;

    if constexpr(!opcode) {
        // 0: ADD  Rd,PC,#nn    ;Rd = (($+4) AND NOT 2) + nn
        // TODO: why do we have to use +2 to pass the tests, instead of the +4 in gbatek?
        uint32_t value = ((cpu->getRegister(PC_REGISTER) + 2) & (~2)) + offset;
        cpu->setRegister(rd, value);
    } else {
        // 1: ADD  Rd,SP,#nn    ;Rd = SP + nn
        uint32_t value = cpu->getRegister(SP_REGISTER) + offset;
        cpu->setRegister(rd, value);
    }

    return SEQUENTIAL;
}
