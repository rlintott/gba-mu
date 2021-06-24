#include <bitset>

#include "ARM7TDMI.h"
#include "Bus.h"
#include "assert.h"

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::shiftHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xE000) == 0);

    uint8_t opcode = (instruction & 0x1800) >> 11;
    uint8_t offset = (instruction & 0x07C0) >> 6;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);

    uint32_t rsVal = cpu->getRegister(rs);

    bool carryFlag;
    uint32_t result;

    switch (opcode) {
        case 0: {
            // 00b: LSL{S} Rd,Rs,#Offset (logical/arithmetic shift left)
            if (offset == 0) {
                result = rsVal;
                carryFlag = cpu->cpsr.C;
            } else {
                result = cpu->aluShiftLsl(rsVal, offset);
                // TODO: put the logic for shift carry bit into functions
                carryFlag = (rsVal >> (32 - offset)) & 1;
            }
            break;
        }
        case 1: {
            // 01b: LSR{S} Rd,Rs,#Offset (logical shift right)
            if (offset == 0) {
                result = cpu->aluShiftLsr(rsVal, 32);
            } else {
                result = cpu->aluShiftLsr(rsVal, offset);
            }
            carryFlag = (rsVal >> (offset - 1)) & 1;
            break;
        }
        case 2: {
            // 10b: ASR{S} Rd,Rs,#Offset (arithmetic shift right)
            if (offset == 0) {
                result = cpu->aluShiftAsr(rsVal, 32);
            } else {
                result = cpu->aluShiftAsr(rsVal, offset);
            }
            carryFlag = (rsVal >> (offset - 1)) & 1;
            break;
        }
        case 3: {
            // 11b: Reserved (used for add/subtract instructions)
            assert(false);
            break;
        }

            cpu->setRegister(rd, result);

            cpu->cpsr.C = carryFlag;
            cpu->cpsr.Z = (result == 0);
            cpu->cpsr.N = (result & 0x80000000);
    }
    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::addSubHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xF800) == 0x1800);
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rs = thumbGetRs(instruction);
    uint32_t op2;
    uint32_t result;
    uint32_t rsVal = cpu->getRegister(rs);

    bool carryFlag;
    bool overflowFlag;
    bool zeroFlag;
    bool signFlag;

    uint8_t opcode = (instruction & 0x0600) >> 9;

    switch (opcode) {
        case 0: {
            // 0: ADD{S} Rd,Rs,Rn   ;add register Rd=Rs+Rn
            op2 = cpu->getRegister((instruction & 0x01C0) >> 6);
            result = rsVal + op2;
            carryFlag = aluAddSetsCarryBit(rsVal, op2);
            overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 1: {
            // 1: SUB{S} Rd,Rs,Rn   ;subtract register   Rd=Rs-Rn
            op2 = cpu->getRegister((instruction & 0x01C0) >> 6);
            result = rsVal - op2;
            carryFlag = aluSubtractSetsCarryBit(rsVal, op2);
            overflowFlag = aluSubtractSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 2: {
            // 2: ADD{S} Rd,Rs,#nn  ;add immediate       Rd=Rs+nn
            op2 = (instruction & 0x01C0) >> 6;
            result = rsVal + op2;
            carryFlag = aluAddSetsCarryBit(rsVal, op2);
            overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 3: {
            // 3: SUB{S} Rd,Rs,#nn  ;subtract immediate  Rd=Rs-nn
            op2 = (instruction & 0x01C0) >> 6;
            result = rsVal - op2;
            carryFlag = aluSubtractSetsCarryBit(rsVal, op2);
            overflowFlag = aluSubtractSetsOverflowBit(rsVal, op2, result);
            break;
        }
    }
    signFlag = aluSetsSignBit(result);
    zeroFlag = aluSetsZeroBit(result);

    cpu->setRegister(rd, result);
    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.N = signFlag;
    cpu->cpsr.V = overflowFlag;

    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::immHandler(uint16_t instruction,
                                                           ARM7TDMI *cpu) {
    assert((instruction & 0xE000) == 0x2000);
    uint8_t opcode = (instruction & 0x1800) >> 11;

    uint32_t offset = instruction & 0x00FF;
    uint8_t rd = (instruction & 0x0700) >> 8;

    bool carryFlag;
    bool signFlag;
    bool overflowFlag;
    bool zeroFlag;

    switch (opcode) {
        case 0: {
            // 00b: MOV{S} Rd,#nn      ;move     Rd   = #nn
            cpu->setRegister(rd, offset);
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            break;
        }
        case 1: {
            // 01b: CMP{S} Rd,#nn      ;compare  Void = Rd - #nn
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result = rdVal - offset;
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
            break;
        }
        case 2: {
            // 10b: ADD{S} Rd,#nn      ;add      Rd   = Rd + #nn
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result = rdVal + offset;
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = aluAddSetsCarryBit(rdVal, offset);
            overflowFlag = aluAddSetsOverflowBit(rdVal, offset, result);
            cpu->setRegister(rd, result);
            break;
        }
        case 3: {
            // 11b: SUB{S} Rd,#nn      ;subtract Rd   = Rd - #nn
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result = rdVal - offset;
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
            cpu->setRegister(rd, result);
            break;
        }
    }

    cpu->cpsr.C = carryFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.N = signFlag;
    cpu->cpsr.V = overflowFlag;

    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::aluHandler(uint16_t instruction,
                                                           ARM7TDMI *cpu) {
    assert((instruction & 0xFC00) == 0x4000);

    uint8_t opcode = (instruction & 0x03C0) >> 6;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    bool carryFlag, overflowFlag, signFlag, zeroFlag;

    switch (opcode) {
        case 0: {
            // 0: AND{S} Rd,Rs     ;AND logical       Rd = Rd AND Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal & rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 1: {
            //  1: EOR{S} Rd,Rs     ;XOR logical       Rd = Rd XOR Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal ^ rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 2: {
            // 2: LSL{S} Rd,Rs     ;log. shift left   Rd = Rd << (Rs AND 0FFh)
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftLsl(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO:
            carryFlag = !(offset) ? cpu->cpsr.C : (rdVal >> (32 - offset)) & 1;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 3: {
            // 3: LSR{S} Rd,Rs     ;log. shift right  Rd = Rd >> (Rs AND 0FFh)
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftLsr(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO:
            carryFlag = !(offset) ? cpu->cpsr.C : (rdVal >> (offset - 1)) & 1;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 4: {
            // 4: ASR{S} Rd,Rs     ;arit shift right  Rd = Rd SAR (Rs AND 0FFh)
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftAsr(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO:
            carryFlag = !(offset) ? cpu->cpsr.C : (rdVal >> (offset - 1)) & 1;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 5: {
            // 5: ADC{S} Rd,Rs     ;add with carry    Rd = Rd + Rs + Cy
            uint64_t rsVal = cpu->getRegister(rs);
            uint64_t rdVal = cpu->getRegister(rd);
            uint64_t result;
            result = rdVal + rsVal + (uint64_t)(cpu->cpsr.C);
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluAddWithCarrySetsCarryBit(result);
            overflowFlag =
                aluAddWithCarrySetsOverflowBit(rdVal, rsVal, result, cpu);
            cpu->setRegister(rd, result);
            break;
        }
        case 6: {
            // 6: SBC{S} Rd,Rs     ;sub with carry    Rd = Rd - Rs - NOT Cy
            uint64_t rsVal = cpu->getRegister(rs);
            uint64_t rdVal = cpu->getRegister(rd);
            uint64_t result;
            result = rdVal - rsVal - !(uint64_t)(cpu->cpsr.C);
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluSubWithCarrySetsCarryBit(result);
            overflowFlag =
                aluSubWithCarrySetsOverflowBit(rdVal, rsVal, result, cpu);
            cpu->setRegister(rd, result);
            break;
        }
        case 7: {
            // 7: ROR{S} Rd,Rs     ;rotate right      Rd = Rd ROR (Rs AND 0FFh)
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftRor(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            overflowFlag = cpu->cpsr.V;
            // TODO:
            carryFlag =
                !(offset) ? cpu->cpsr.C : (rdVal >> ((offset % 32) - 1)) & 1;
            cpu->setRegister(rd, result);
            break;
        }
        case 8: {
            // 8: TST    Rd,Rs     ;test            Void = Rd AND Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal & rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            break;
        }
        case 9: {
            // 9: NEG{S} Rd,Rs     ;negate            Rd = 0 - Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t result;
            result = (uint32_t)0 - rsVal;
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluSubtractSetsCarryBit(0, rsVal);
            overflowFlag = aluSubtractSetsOverflowBit(0, rsVal, result);
            cpu->setRegister(rd, result);
            break;
        }
        case 0xA: {
            // A: CMP    Rd,Rs     ;compare         Void = Rd - Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal - rsVal;
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluSubtractSetsCarryBit(rdVal, rsVal);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, rsVal, result);
            break;
        }
        case 0xB: {
            // B: CMN    Rd,Rs     ;neg.compare     Void = Rd + Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal + rsVal;
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluAddSetsCarryBit(rdVal, rsVal);
            overflowFlag = aluAddSetsOverflowBit(rdVal, rsVal, result);
            break;
        }
        case 0xC: {
            // C: ORR{S} Rd,Rs     ;OR logical        Rd = Rd OR Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal | rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 0xD: {
            // D: MUL{S} Rd,Rs     ;multiply          Rd = Rd * Rs
            uint64_t rsVal = cpu->getRegister(rs);
            uint64_t rdVal = cpu->getRegister(rd);
            uint64_t result;
            result = rdVal * rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // carry flag destroyed
            carryFlag = 0;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 0xE: {
            // E: BIC{S} Rd,Rs     ;bit clear         Rd = Rd AND NOT Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t rdVal = cpu->getRegister(rd);
            uint32_t result;
            result = rdVal & (~rsVal);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
        case 0xF: {
            // F: MVN{S} Rd,Rs     ;not               Rd = NOT Rs
            uint32_t rsVal = cpu->getRegister(rs);
            uint32_t result;
            result = ~rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpu->cpsr.C;
            overflowFlag = cpu->cpsr.V;
            cpu->setRegister(rd, result);
            break;
        }
    }

    cpu->cpsr.N = signFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.C = carryFlag;
    cpu->cpsr.V = overflowFlag;
    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::bxHandler(uint16_t instruction,
                                                          ARM7TDMI *cpu) {
    assert((instruction & 0xFC00) == 0x4400);
    uint8_t opcode = (instruction & 0x0300) >> 8;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    uint8_t msbd =
        (instruction & 0x0080) >>
        7;  // MSBd - Destination Register most significant bit (or BL/BLX flag)
    uint8_t msbs = (instruction & 0x0040) >>
                   6;  // MSBs - Source Register most significant bit

    rs = rs & (msbs << 3);
    rd = rd & (msbd << 3);

    uint32_t rsVal = cpu->getRegister(rs);
    uint32_t rdVal = cpu->getRegister(rd);
    rsVal = (rsVal == PC_REGISTER) ? rsVal + 4 : rsVal;
    rdVal = (rdVal == PC_REGISTER) ? rdVal + 4 : rdVal;

    switch (opcode) {
        case 0: {
            // 0: ADD Rd,Rs   ;add        Rd = Rd+Rs
            assert(msbd || msbs);
            uint32_t result = rdVal + rsVal;
            cpu->setRegister(rd, result);
            break;
        }
        case 1: {
            // 1: CMP Rd,Rs   ;compare  Void = Rd-Rs  ;CPSR affected
            assert(msbd || msbs);
            uint32_t result;
            result = rdVal - rsVal;
            cpu->cpsr.N = aluSetsSignBit((uint32_t)result);
            cpu->cpsr.Z = aluSetsZeroBit((uint32_t)result);
            cpu->cpsr.C = aluSubtractSetsCarryBit(rdVal, rsVal);
            cpu->cpsr.V = aluSubtractSetsOverflowBit(rdVal, rsVal, result);
            break;
        }
        case 2: {
            // 2: MOV Rd,Rs   ;move       Rd = Rs
            cpu->setRegister(rd, rsVal);
            assert(msbd || msbs);
            break;
        }
        case 3: {
            // 3: BX  Rs      ;jump       PC = Rs     ;may switch THUMB/ARM
            if (rs == PC_REGISTER || !(rsVal & 0x1)) {
                // R15: CPU switches to ARM state, and PC is auto-aligned as
                // (($+4) AND NOT 2).

                // For BX/BLX, when Bit 0 of the value in Rs is zero:
                // Processor will be switched into ARM mode!
                // If so, Bit 1 of Rs must be cleared (32bit word aligned).
                // Thus, BX PC (switch to ARM) may be issued from word-aligned
                // address only, the destination is PC+4 (ie. the following
                // halfword is skipped).
                cpu->cpsr.T = 0;
                rsVal &= 0xFFFFFFFD;
            }

            assert(msbd == 0);
            rsVal &= 0xFFFFFFFE;
            cpu->setRegister(PC_REGISTER, rsVal);
            break;
        }
    }
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::loadPcRelativeHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xF800) == 0x4800);
    uint8_t offset = (instruction & 0x00FF) << 2;
    uint8_t rd = (instruction & 0x0700) >> 8;
    uint32_t address =
        ((cpu->getRegister(PC_REGISTER) + 4) & 0xFFFFFFFD) + offset;
    uint32_t value =
        aluShiftRor(cpu->bus->read32(address & 0xFFFFFFFC), (address & 3) * 8);
    cpu->setRegister(rd, value);

    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::loadStoreRegOffsetHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xF000) == 0x5000);
    assert(!(instruction & 0x0200));
    uint8_t opcode = (instruction & 0x0C00) >> 10;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    uint8_t ro = (instruction & 0x01C0) >> 6;
    uint32_t address = cpu->getRegister(rb) + cpu->getRegister(ro);

    switch (opcode) {
        case 0: {
            // 0: STR  Rd,[Rb,Ro]   ;store 32bit data  WORD[Rb+Ro] = Rd
            cpu->bus->write32(address & 0xFFFFFFFC, cpu->getRegister(rd));
            break;
        }
        case 1: {
            // 1: STRB Rd,[Rb,Ro]   ;store  8bit data  BYTE[Rb+Ro] = Rd
            cpu->bus->write8(address, (uint8_t)(cpu->getRegister(rd)));
            break;
        }
        case 2: {
            // 2: LDR  Rd,[Rb,Ro]   ;load  32bit data  Rd = WORD[Rb+Ro]
            uint32_t value = aluShiftRor(cpu->bus->read32(address & 0xFFFFFFFC),
                                         (address & 3) * 8);
            cpu->setRegister(rd, value);
            break;
        }
        case 3: {
            // 3: LDRB Rd,[Rb,Ro]   ;load   8bit data  Rd = BYTE[Rb+Ro]
            uint32_t value = (uint32_t)cpu->bus->read8(address);
            cpu->setRegister(rd, value);
            break;
        }
    }
    return {};
}

ARM7TDMI::Cycles
ARM7TDMI::ThumbOpcodeHandlers::loadStoreSignExtendedByteHalfwordHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xF000) == 0x5000);
    assert(instruction & 0x0200);
    uint8_t opcode = (instruction & 0x0C00) >> 10;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    uint8_t ro = (instruction & 0x01C0) >> 6;
    uint32_t address = cpu->getRegister(rb) + cpu->getRegister(ro);

    switch (opcode) {
        case 0: {
            // 0: STRH Rd,[Rb,Ro]  ;store 16bit data          HALFWORD[Rb+Ro] =
            // Rd
            cpu->bus->write16(address & 0xFFFFFFFE, cpu->getRegister(rd));
            break;
        }
        case 1: {
            // 1: LDSB Rd,[Rb,Ro]  ;load sign-extended 8bit   Rd = BYTE[Rb+Ro]
            uint32_t value = cpu->bus->read8(address);
            value = (value & 0x80) ? (0xFFFFFF00 & value) : value;
            cpu->setRegister(rd, value);
            break;
        }
        case 2: {
            // 2: LDRH Rd,[Rb,Ro]  ;load zero-extended 16bit  Rd =
            // HALFWORD[Rb+Ro]
            uint32_t value = aluShiftRor(cpu->bus->read16(address & 0xFFFFFFFE),
                                         (address & 1) * 8);
            cpu->setRegister(rd, value);
            break;
        }
        case 3: {
            // 3: LDSH Rd,[Rb,Ro]  ;load sign-extended 16bit  Rd =
            // HALFWORD[Rb+Ro]
            uint32_t value = aluShiftRor(cpu->bus->read16(address & 0xFFFFFFFE),
                                         (address & 1) * 8);
            value = (value & 0x8000) ? (0xFFFF0000 & value) : value;
            cpu->setRegister(rd, value);
            break;
        }
    }
    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::loadStoreImmediateOffsetHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xE000) == 0x6000);
    uint8_t opcode = (instruction & 0x1800) >> 11;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);

    switch (opcode) {
        case 0: {
            // 0: STR  Rd,[Rb,#nn]  ;store 32bit data   WORD[Rb+nn] = Rd
            uint32_t offset = (instruction & 0x07C0) >> 4;
            uint32_t address = cpu->getRegister(rb) + offset;
            cpu->bus->write32(address & 0xFFFFFFFC, cpu->getRegister(rd));
            break;
        }
        case 1: {
            // 1: LDR  Rd,[Rb,#nn]  ;load  32bit data   Rd = WORD[Rb+nn]
            uint32_t offset = (instruction & 0x07C0) >> 4;
            uint32_t address = cpu->getRegister(rb) + offset;
            uint32_t value = aluShiftRor(cpu->bus->read32(address & 0xFFFFFFFC),
                                         (address & 3) * 8);
            cpu->setRegister(rd, value);
            break;
        }
        case 2: {
            // 2: STRB Rd,[Rb,#nn]  ;store  8bit data   BYTE[Rb+nn] = Rd
            uint32_t offset = (instruction & 0x07C0) >> 6;
            uint32_t address = cpu->getRegister(rb) + offset;
            cpu->bus->write8(address, (uint8_t)(cpu->getRegister(rd)));
            break;
        }
        case 3: {
            // 3: LDRB Rd,[Rb,#nn]  ;load   8bit data   Rd = BYTE[Rb+nn]
            uint32_t offset = (instruction & 0x07C0) >> 6;
            uint32_t address = cpu->getRegister(rb) + offset;
            cpu->setRegister(rd, (uint32_t)cpu->bus->read8(address));
            break;
        }
    }
    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::loadStoreHalfwordHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xF000) == 0x8000);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    uint32_t offset = (instruction & 0x07C0) >> 5;
    uint32_t address = cpu->getRegister(rb) + offset;

    switch (opcode) {
        case 0: {
            // 0: STRH Rd,[Rb,#nn]  ;store 16bit data   HALFWORD[Rb+nn] = Rd
            cpu->bus->write16(address & 0xFFFFFFFE,
                              (uint16_t)cpu->getRegister(rd));
            break;
        }
        case 1: {
            // 1: LDRH Rd,[Rb,#nn]  ;load  16bit data   Rd = HALFWORD[Rb+nn]
            uint32_t value = aluShiftRor(cpu->bus->read16(address & 0xFFFFFFFE),
                                         (address & 1) * 8);
            cpu->setRegister(rd, value);
            break;
        }
    }
    return {};
}

ARM7TDMI::Cycles ARM7TDMI::ThumbOpcodeHandlers::loadStoreSpRelativeHandler(
    uint16_t instruction, ARM7TDMI *cpu) {
    assert((instruction & 0xF000) == 0x9000);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    uint8_t rd = (instruction & 0x0700) >> 8;
    uint16_t offset = (instruction & 0x00FF) << 2;
    uint32_t address = cpu->getRegister(SP_REGISTER) + offset;

    switch (opcode) {
        case 0: {
            // 0: STR  Rd,[SP,#nn]  ;store 32bit data   WORD[SP+nn] = Rd
            cpu->bus->write32(address & 0xFFFFFFFC, cpu->getRegister(rd));
            break;
        }
        case 1: {
            // 1: LDR  Rd,[SP,#nn]  ;load  32bit data   Rd = WORD[SP+nn]
            uint32_t value = aluShiftRor(cpu->bus->read32(address & 0xFFFFFFFC),
                                         (address & 3) * 8);
            cpu->setRegister(rd, value);
            break;
        }
    }
    return {};
}
