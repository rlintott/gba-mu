#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbShiftHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xE000) == 0);

    //uint8_t opcode = (instruction & 0x1800) >> 11;
    constexpr uint8_t opcode = (op & 0x060) >> 5;
    uint8_t offset = (instruction & 0x07C0) >> 6;
    //constexpr uint8_t offset = (op & 0x01F);
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);

    uint32_t rsVal = cpu->getRegister(rs);

    bool carryFlag;
    uint32_t result;

    if constexpr(opcode == 0) {

        // 00b: LSL{S} Rd,Rs,#Offset (logical/arithmetic shift left)
        if(offset == 0) {
            result = rsVal;
            carryFlag = cpu->cpsr.C;
        } else {
            result = aluShiftLsl(rsVal, offset);
            // TODO: put the logic for shift carry bit into functions
            carryFlag = (rsVal >> (32 - offset)) & 1;
        }
    } else if constexpr(opcode == 1) {
        // 01b: LSR{S} Rd,Rs,#Offset (logical shift right)
        if(offset == 0) {
            result = aluShiftLsr(rsVal, 32);
        } else {
            result = aluShiftLsr(rsVal, offset);
        }
        carryFlag = (rsVal >> (offset - 1)) & 1;
    } else if constexpr(opcode == 2) {

        // 10b: ASR{S} Rd,Rs,#Offset (arithmetic shift right)
        if(offset == 0) {
            result = aluShiftAsr(rsVal, 32);
        } else {
            result = aluShiftAsr(rsVal, offset);
        }
        carryFlag = (rsVal >> (offset - 1)) & 1;
    } else {
        // opcode == 3
        // 11b: Reserved (used for add/subtract instructions)
        assert(false);
    }

    cpu->setRegister(rd, result);
    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = (result == 0);
    cpu->cpsr.N = (bool)(result & 0x80000000);
   
    return SEQUENTIAL;
}