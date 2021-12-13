#include "../ARM7TDMI.h"
#include "../Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbCondBHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0xD000);
    DEBUG("in THUMB.16: conditional branch\n");
    //uint8_t opcode = (instruction & 0x0F00) >> 8;
    constexpr uint8_t opcode = (op & 0x03C) >> 2;
    uint32_t offset = signExtend9Bit((instruction & 0x00FF) << 1);
    bool jump = false;

    DEBUG("opcode: " << (uint64_t)opcode << "\n");

    // Destination address must by halfword aligned (ie. bit 0 cleared)
    // Return: No flags affected, PC adjusted if condition true

    if constexpr(opcode == 0x0) {
        // 0: BEQ label        ;Z=1         ;equal (zero) (same)
        jump = cpu->cpsr.Z;
    } else if constexpr(opcode == 0x1) {
        // 1: BNE label        ;Z=0         ;not equal (nonzero) (not same)
        jump = !(cpu->cpsr.Z);
    } else if constexpr(opcode == 0x2) {
        // 2: BCS/BHS label    ;C=1         ;unsigned higher or same (carry set)
        jump = cpu->cpsr.C;
    } else if constexpr(opcode == 0x3) {
        // 3: BCC/BLO label    ;C=0         ;unsigned lower (carry cleared)
        jump = !(cpu->cpsr.C);
    } else if constexpr(opcode == 0x4) {
        // 4: BMI label        ;N=1         ;signed  negative (minus)
        jump = cpu->cpsr.N;
    } else if constexpr(opcode == 0x5) {
        // 5: BPL label        ;N=0         ;signed  positive or zero (plus)
        jump = !(cpu->cpsr.N);
    } else if constexpr(opcode == 0x6) {
        // 6: BVS label        ;V=1         ;signed  overflow (V set)
        jump = cpu->cpsr.V;
    } else if constexpr(opcode == 0x7) {
        // 7: BVC label        ;V=0         ;signed  no overflow (V cleared)
        jump = !(cpu->cpsr.V);
    } else if constexpr(opcode == 0x8) {
        // 8: BHI label        ;C=1 and Z=0 ;unsigned higher
        jump = !(cpu->cpsr.Z) && (cpu->cpsr.C);
    } else if constexpr(opcode == 0x9) {
        // 9: BLS label        ;C=0 or Z=1  ;unsigned lower or same
        jump = !(cpu->cpsr.C) || cpu->cpsr.Z;
    } else if constexpr(opcode == 0xA) {
        // A: BGE label        ;N=V         ;signed greater or equal
        jump = cpu->cpsr.N == cpu->cpsr.V;
    } else if constexpr(opcode == 0xB) {
        // B: BLT label        ;N<>V        ;signed less than
        jump = cpu->cpsr.N != cpu->cpsr.V;
    } else if constexpr(opcode == 0xC) {
        // C: BGT label        ;Z=0 and N=V ;signed greater than
        jump = !(cpu->cpsr.Z) && (cpu->cpsr.N == cpu->cpsr.V);
    } else if constexpr(opcode == 0xD) {
        // D: BLE label        ;Z=1 or N<>V ;signed less or equal
        jump = (cpu->cpsr.Z) || (cpu->cpsr.N != cpu->cpsr.V);
    } else if constexpr(opcode == 0xE) {
        // E: Undefined, should not be used
        assert(false);
    } else {
        // opcode == 0xF
        //  F: Reserved for SWI instruction (see SWI opcode)
        assert(false);
    }

    if(jump) {
        cpu->setRegister(PC_REGISTER, (cpu->getRegister(PC_REGISTER) + 2 + offset) & 0xFFFFFFFE);
        return BRANCH;
    } else {
        return SEQUENTIAL;
    }
}
