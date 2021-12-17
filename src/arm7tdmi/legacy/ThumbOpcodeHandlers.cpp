#include <bitset>

#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"
#include "assert.h"

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::shiftHandler(uint16_t instruction) {
    assert((instruction & 0xE000) == 0);

    uint8_t opcode = (instruction & 0x1800) >> 11;
    uint8_t offset = (instruction & 0x07C0) >> 6;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);

    uint32_t rsVal = getRegister(rs);

    bool carryFlag;
    uint32_t result;

    switch (opcode) {
        case 0: {
            // 00b: LSL{S} Rd,Rs,#Offset (logical/arithmetic shift left)
            if (offset == 0) {
                result = rsVal;
                carryFlag = cpsr.C;
            } else {
                result = aluShiftLsl(rsVal, offset);
                // TODO: put the logic for shift carry bit into functions
                carryFlag = (rsVal >> (32 - offset)) & 1;
            }
            break;
        }
        case 1: {
            // 01b: LSR{S} Rd,Rs,#Offset (logical shift right)
            if (offset == 0) {
                result = aluShiftLsr(rsVal, 32);
            } else {
                result = aluShiftLsr(rsVal, offset);
            }
            carryFlag = (rsVal >> (offset - 1)) & 1;
            break;
        }
        case 2: {
            // 10b: ASR{S} Rd,Rs,#Offset (arithmetic shift right)
            if (offset == 0) {
                result = aluShiftAsr(rsVal, 32);
            } else {
                result = aluShiftAsr(rsVal, offset);
            }
            carryFlag = (rsVal >> (offset - 1)) & 1;
            break;
        }
        case 3: {
            // 11b: Reserved (used for add/subtract instructions)
            assert(false);
            break;
        }

    }
    setRegister(rd, result);
    cpsr.C = carryFlag;
    cpsr.Z = (result == 0);
    cpsr.N = (bool)(result & 0x80000000);
   
    return SEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::addSubHandler(uint16_t instruction) {
    assert((instruction & 0xF800) == 0x1800);
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rs = thumbGetRs(instruction);
    uint32_t op2;
    uint32_t result;
    uint32_t rsVal = getRegister(rs);

    bool carryFlag;
    bool overflowFlag;
    bool zeroFlag;
    bool signFlag;

    uint8_t opcode = (instruction & 0x0600) >> 9;

    switch (opcode) {
        case 0: {
            // 0: ADD{S} Rd,Rs,Rn   ;add register Rd=Rs+Rn
            op2 = getRegister((instruction & 0x01C0) >> 6);
            result = rsVal + op2;
            carryFlag = aluAddSetsCarryBit(rsVal, op2);
            overflowFlag = aluAddSetsOverflowBit(rsVal, op2, result);
            break;
        }
        case 1: {
            // 1: SUB{S} Rd,Rs,Rn   ;subtract register   Rd=Rs-Rn
            op2 = getRegister((instruction & 0x01C0) >> 6);
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

    setRegister(rd, result);
    cpsr.C = carryFlag;
    cpsr.Z = zeroFlag;
    cpsr.N = signFlag;
    cpsr.V = overflowFlag;
  
    return SEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::immHandler(uint16_t instruction) {
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
            setRegister(rd, offset);
            signFlag = aluSetsSignBit(offset);
            zeroFlag = aluSetsZeroBit(offset);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            break;
        }
        case 1: {
            // 01b: CMP{S} Rd,#nn      ;compare  Void = Rd - #nn
            uint32_t rdVal = getRegister(rd);
            uint32_t result = rdVal - offset;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
            break;
        }
        case 2: {
            // 10b: ADD{S} Rd,#nn      ;add      Rd   = Rd + #nn
            uint32_t rdVal = getRegister(rd);
            uint32_t result = rdVal + offset;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = aluAddSetsCarryBit(rdVal, offset);
            overflowFlag = aluAddSetsOverflowBit(rdVal, offset, result);
            setRegister(rd, result);
            break;
        }
        case 3: {
            // 11b: SUB{S} Rd,#nn      ;subtract Rd   = Rd - #nn
            uint32_t rdVal = getRegister(rd);
            uint32_t result = rdVal - offset;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = aluSubtractSetsCarryBit(rdVal, offset);
            overflowFlag = aluSubtractSetsOverflowBit(rdVal, offset, result);
            setRegister(rd, result);
            break;
        }
    }

    cpsr.C = carryFlag;
    cpsr.Z = zeroFlag;
    cpsr.N = signFlag;
    cpsr.V = overflowFlag;

    return SEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::aluHandler(uint16_t instruction) {
    assert((instruction & 0xFC00) == 0x4000);
    uint8_t opcode = (instruction & 0x03C0) >> 6;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    bool carryFlag, overflowFlag, signFlag, zeroFlag;

    Cycles cycles;
    int internalCycles = 0;

    switch (opcode) {
        case 0: {
            // 0: AND{S} Rd,Rs     ;AND logical       Rd = Rd AND Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint32_t result;
            result = rdVal & rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            break;
        }
        case 1: {
            //  1: EOR{S} Rd,Rs     ;XOR logical       Rd = Rd XOR Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint32_t result;
            result = rdVal ^ rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            break;
        }
        case 2: {
            // 2: LSL{S} Rd,Rs     ;log. shift left   Rd = Rd << (Rs AND 0FFh)
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftLsl(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO:
            carryFlag = !(offset) ? cpsr.C : (rdVal >> (32 - offset)) & 1;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            internalCycles = 1;
            break;
        }
        case 3: {
            // 3: LSR{S} Rd,Rs     ;log. shift right  Rd = Rd >> (Rs AND 0FFh)
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftLsr(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO:
            carryFlag = !(offset) ? cpsr.C : (rdVal >> (offset - 1)) & 1;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            internalCycles = 1;
            break;
        }
        case 4: {
            // 4: ASR{S} Rd,Rs     ;arit shift right  Rd = Rd SAR (Rs AND 0FFh)
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftAsr(rdVal, offset);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // TODO:
            carryFlag = !(offset) ? cpsr.C : (rdVal >> (offset - 1)) & 1;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            internalCycles = 1;
            break;
        }
        case 5: {
            // 5: ADC{S} Rd,Rs     ;add with carry    Rd = Rd + Rs + Cy
            uint64_t rsVal = getRegister(rs);
            uint64_t rdVal = getRegister(rd);
            uint64_t result;
            result = rdVal + rsVal + (uint64_t)(cpsr.C);
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluAddWithCarrySetsCarryBit(result);
            overflowFlag =
                aluAddWithCarrySetsOverflowBit(rdVal, rsVal, result, this);
            setRegister(rd, result);
            break;
        }
        case 6: {
            // 6: SBC{S} Rd,Rs     ;sub with carry    Rd = Rd - Rs - NOT Cy
            uint64_t rsVal = getRegister(rs);
            uint64_t rdVal = getRegister(rd);
            uint64_t result;
            result = rdVal - rsVal - !(uint64_t)(cpsr.C);
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluSubWithCarrySetsCarryBit(result);
            overflowFlag =
                aluSubWithCarrySetsOverflowBit(rdVal, rsVal, result, this);
            setRegister(rd, result);
            break;
        }
        case 7: {
            // 7: ROR{S} Rd,Rs     ;rotate right      Rd = Rd ROR (Rs AND 0FFh)
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint8_t offset = rsVal & 0xFF;
            uint32_t result;
            result = aluShiftRor(rdVal, offset % 32);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            overflowFlag = cpsr.V;
            // TODO:
            carryFlag =
                !(offset) ? cpsr.C : (rdVal >> ((offset % 32) - 1)) & 1;
            setRegister(rd, result);
            internalCycles = 1;
            break;
        }
        case 8: {
            // 8: TST    Rd,Rs     ;test            Void = Rd AND Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint32_t result;
            result = rdVal & rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            break;
        }
        case 9: {
            // 9: NEG{S} Rd,Rs     ;negate            Rd = 0 - Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t result;
            result = (uint32_t)0 - rsVal;
            signFlag = aluSetsSignBit((uint32_t)result);
            zeroFlag = aluSetsZeroBit((uint32_t)result);
            carryFlag = aluSubtractSetsCarryBit(0, rsVal);
            overflowFlag = aluSubtractSetsOverflowBit(0, rsVal, result);
            setRegister(rd, result);
            break;
        }
        case 0xA: {
            // A: CMP    Rd,Rs     ;compare         Void = Rd - Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
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
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
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
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint32_t result;
            result = rdVal | rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            break;
        }
        case 0xD: {
            // D: MUL{S} Rd,Rs     ;multiply          Rd = Rd * Rs
            uint64_t rsVal = getRegister(rs);
            uint64_t rdVal = getRegister(rd);
            uint64_t result;
            result = rdVal * rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            // carry flag destroyed
            // TODO: why? it says on GBATek ARMv4 and below: carry flag destroyed
            carryFlag = cpsr.C;
            // carryFlag = 0;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            internalCycles = mulGetExecutionTimeMVal(rdVal);
            break;
        }
        case 0xE: {
            // E: BIC{S} Rd,Rs     ;bit clear         Rd = Rd AND NOT Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t rdVal = getRegister(rd);
            uint32_t result;
            result = rdVal & (~rsVal);
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            break;
        }
        case 0xF: {
            // F: MVN{S} Rd,Rs     ;not               Rd = NOT Rs
            uint32_t rsVal = getRegister(rs);
            uint32_t result;
            result = ~rsVal;
            signFlag = aluSetsSignBit(result);
            zeroFlag = aluSetsZeroBit(result);
            carryFlag = cpsr.C;
            overflowFlag = cpsr.V;
            setRegister(rd, result);
            break;
        }
    }

    cpsr.N = signFlag;
    cpsr.Z = zeroFlag;
    cpsr.C = carryFlag;
    cpsr.V = overflowFlag;

    for(int i = 0; i < internalCycles; i++) {
        bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }
    if(!internalCycles) {
        return SEQUENTIAL;
    } else {
        return NONSEQUENTIAL;
    }
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::bxHandler(uint16_t instruction) {
    assert((instruction & 0xFC00) == 0x4400);
    uint8_t opcode = (instruction & 0x0300) >> 8;
    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    uint8_t msbd =
        (instruction & 0x0080) >>
        7;  // MSBd - Destination Register most significant bit (or BL/BLX flag)
    uint8_t msbs = (instruction & 0x0040) >>
                   6;  // MSBs - Source Register most significant bit


    rs = rs | (msbs << 3);
    rd = rd | (msbd << 3);

    uint32_t rsVal = getRegister(rs);
    uint32_t rdVal = getRegister(rd);
    rsVal = (rs == PC_REGISTER) ? (rsVal + 2) & 0xFFFFFFFE : rsVal;
    rdVal = (rd == PC_REGISTER) ? (rdVal + 2) & 0xFFFFFFFE : rdVal;

    // TODO: why do we have to do this even if rs != 15? 
    if(rd == PC_REGISTER) {
        rsVal &= 0xFFFFFFFE;
    }

    switch (opcode) {
        case 0: {
            // 0: ADD Rd,Rs   ;add        Rd = Rd+Rs
            assert(msbd || msbs);
            uint32_t result = rdVal + rsVal;
            setRegister(rd, result);
            if(rd == PC_REGISTER) {
                return BRANCH;
            } else {
                return SEQUENTIAL;
            }
        }
        case 1: {
            // 1: CMP Rd,Rs   ;compare  Void = Rd-Rs  ;CPSR affected
            assert(msbd || msbs);
            uint32_t result;
            result = rdVal - rsVal;
            cpsr.N = aluSetsSignBit((uint32_t)result);
            cpsr.Z = aluSetsZeroBit((uint32_t)result);
            cpsr.C = aluSubtractSetsCarryBit(rdVal, rsVal);
            cpsr.V = aluSubtractSetsOverflowBit(rdVal, rsVal, result);
            return BRANCH;
        }
        case 2: {
            // 2: MOV Rd,Rs   ;move       Rd = Rs
            setRegister(rd, rsVal);
    
            assert(msbd || msbs);
            if(rd == PC_REGISTER) {
                return BRANCH;
            } else {
                return SEQUENTIAL;
            }
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
                cpsr.T = 0;
                rsVal &= 0xFFFFFFFD;
            }

            assert(msbd == 0);
            rsVal &= 0xFFFFFFFE;
            setRegister(PC_REGISTER, rsVal);

            return BRANCH;
        }
    }

}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::loadPcRelativeHandler(uint16_t instruction) {
    assert((instruction & 0xF800) == 0x4800);
    uint16_t offset = (instruction & 0x00FF) << 2;
    uint8_t rd = (instruction & 0x0700) >> 8;
    uint32_t address =
        ((getRegister(PC_REGISTER) + 2) & 0xFFFFFFFD) + offset;
    uint32_t value = aluShiftRor(bus->read32(address & 0xFFFFFFFC, 
                                 Bus::CycleType::NONSEQUENTIAL), 
                                 (address & 3) * 8);
    setRegister(rd, value);
    bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::loadStoreRegOffsetHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0x5000);
    assert(!(instruction & 0x0200));
    uint8_t opcode = (instruction & 0x0C00) >> 10;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    uint8_t ro = (instruction & 0x01C0) >> 6;
    uint32_t address = getRegister(rb) + getRegister(ro);

    switch (opcode) {
        case 0: {
            // 0: STR  Rd,[Rb,Ro]   ;store 32bit data  WORD[Rb+Ro] = Rd
            bus->write32(address & 0xFFFFFFFC, getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 1: {
            // 1: STRB Rd,[Rb,Ro]   ;store  8bit data  BYTE[Rb+Ro] = Rd
            bus->write8(address, (uint8_t)(getRegister(rd)), Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 2: {
            // 2: LDR  Rd,[Rb,Ro]   ;load  32bit data  Rd = WORD[Rb+Ro]
            uint32_t value = aluShiftRor(bus->read32(address & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL),
                                         (address & 3) * 8);
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
            break;
        }
        case 3: {
            // 3: LDRB Rd,[Rb,Ro]   ;load   8bit data  Rd = BYTE[Rb+Ro]
            uint32_t value = (uint32_t)bus->read8(address, Bus::CycleType::NONSEQUENTIAL);
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
            break;
        }
    }
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess
ARM7TDMI::loadStoreSignExtendedByteHalfwordHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0x5000);
    assert(instruction & 0x0200);
    uint8_t opcode = (instruction & 0x0C00) >> 10;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    uint8_t ro = (instruction & 0x01C0) >> 6;
    uint32_t address = getRegister(rb) + getRegister(ro);

    Cycles cycles;

    switch (opcode) {
        case 0: {
            // 0: STRH Rd,[Rb,Ro]  ;store 16bit data          HALFWORD[Rb+Ro] =
            // Rd
            bus->write16(address & 0xFFFFFFFE, getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 1: {
            // 1: LDSB Rd,[Rb,Ro]  ;load sign-extended 8bit   Rd = BYTE[Rb+Ro]
            uint32_t value = bus->read8(address, Bus::CycleType::NONSEQUENTIAL);
            value = (value & 0x80) ? (0xFFFFFF00 | value) : value;
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);         
            break;
        }
        case 2: {
            // 2: LDRH Rd,[Rb,Ro]  ;load zero-extended 16bit  Rd =
            // HALFWORD[Rb+Ro]
            uint32_t value = aluShiftRor(bus->read16(address & 0xFFFFFFFE, Bus::CycleType::NONSEQUENTIAL),
                                         (address & 1) * 8);
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
            break;
        }
        case 3: {
            // 3: LDSH Rd,[Rb,Ro]  ;load sign-extended 16bit  Rd = HALFWORD[Rb+Ro]
            if(address & 0x1) {
                // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value
                // if address odd
                uint32_t value = bus->read8(address, Bus::CycleType::NONSEQUENTIAL);
                value = (value & 0x80) ? (0xFFFFFF00 | value) : value;
                setRegister(rd, value);
            } else {
                uint32_t value = aluShiftRor(bus->read16(address & 0xFFFFFFFE, Bus::CycleType::NONSEQUENTIAL),
                                         (address & 1) * 8);
                value = (value & 0x8000) ? (0xFFFF0000 | value) : value;
                setRegister(rd, value);
            }
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0); 
            break;
        }
    }
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::loadStoreImmediateOffsetHandler(uint16_t instruction) {
    assert((instruction & 0xE000) == 0x6000);
    uint8_t opcode = (instruction & 0x1800) >> 11;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);

    switch (opcode) {
        case 0: {
            // 0: STR  Rd,[Rb,#nn]  ;store 32bit data   WORD[Rb+nn] = Rd
            uint32_t offset = (instruction & 0x07C0) >> 4;
            uint32_t address = getRegister(rb) + offset;
            bus->write32(address & 0xFFFFFFFC, getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 1: {
            // 1: LDR  Rd,[Rb,#nn]  ;load  32bit data   Rd = WORD[Rb+nn]
            uint32_t offset = (instruction & 0x07C0) >> 4;
            uint32_t address = getRegister(rb) + offset;
            uint32_t value = aluShiftRor(bus->read32(address & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL),
                                         (address & 3) * 8);
            // if(rd == PC_REGISTER) {
            //     // For THUMB code, the low bit of the target address may/should/must be set,
            //     // the bit is (or is not) interpreted as thumb-bit (depending on the opcode), 
            //     // and R15 is then forcibly aligned by clearing the lower bit.
            //     // TODO: check that this is applied everywhere
            //     setRegister(rd, value & 0xFFFFFFFE);
            // } else {
            //     setRegister(rd, value);
            // }
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
            break;
        }
        case 2: {
            // 2: STRB Rd,[Rb,#nn]  ;store  8bit data   BYTE[Rb+nn] = Rd
            uint32_t offset = (instruction & 0x07C0) >> 6;
            uint32_t address = getRegister(rb) + offset;
            bus->write8(address, (uint8_t)(getRegister(rd)), Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 3: {
            // 3: LDRB Rd,[Rb,#nn]  ;load   8bit data   Rd = BYTE[Rb+nn]
            uint32_t offset = (instruction & 0x07C0) >> 6;
            uint32_t address = getRegister(rb) + offset;
            setRegister(rd, (uint32_t)bus->read8(address, Bus::CycleType::NONSEQUENTIAL));
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
            break;
        }
    }
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::loadStoreHalfwordHandler(uint16_t instruction) {

    assert((instruction & 0xF000) == 0x8000);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    uint32_t offset = (instruction & 0x07C0) >> 5;
    uint32_t address = getRegister(rb) + offset;

    switch (opcode) {
        case 0: {
            // 0: STRH Rd,[Rb,#nn]  ;store 16bit data   HALFWORD[Rb+nn] = Rd
            bus->write16(address & 0xFFFFFFFE,
                              (uint16_t)getRegister(rd), 
                              Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 1: {
            // 1: LDRH Rd,[Rb,#nn]  ;load  16bit data   Rd = HALFWORD[Rb+nn]
            uint32_t value = aluShiftRor(bus->read16(address & 0xFFFFFFFE, 
                                         Bus::CycleType::NONSEQUENTIAL),
                                         (address & 1) * 8);
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);

            break;
        }
    }
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::loadStoreSpRelativeHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0x9000);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    uint8_t rd = (instruction & 0x0700) >> 8;
    uint16_t offset = (instruction & 0x00FF) << 2;
    uint32_t address = getRegister(SP_REGISTER) + offset;

    switch (opcode) {
        case 0: {
            // 0: STR  Rd,[SP,#nn]  ;store 32bit data   WORD[SP+nn] = Rd
            bus->write32(address & 0xFFFFFFFC, getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
            break;
        }
        case 1: {
            // 1: LDR  Rd,[SP,#nn]  ;load  32bit data   Rd = WORD[SP+nn]
            uint32_t value = aluShiftRor(bus->read32(address & 0xFFFFFFFC, 
                                         Bus::CycleType::NONSEQUENTIAL),
                                         (address & 3) * 8);
            setRegister(rd, value);
            bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
            break;
        }
    }
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::getRelativeAddressHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0xA000);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    uint8_t rd = (instruction & 0x0700) >> 8;
    uint16_t offset = (instruction & 0x00FF) << 2;

    switch (opcode) {
        case 0: {
            // 0: ADD  Rd,PC,#nn    ;Rd = (($+4) AND NOT 2) + nn
            // TODO: why do we have to use +2 to pass the tests, instead of the +4 in gbatek?
            uint32_t value = ((getRegister(PC_REGISTER) + 2) & (~2)) + offset;
            setRegister(rd, value);
            break;
        }
        case 1: {
            // 1: ADD  Rd,SP,#nn    ;Rd = SP + nn
            uint32_t value = getRegister(SP_REGISTER) + offset;
            setRegister(rd, value);
            break;
        }
    }
    return  SEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::addOffsetToSpHandler(uint16_t instruction) {
    assert((instruction & 0xFF00) == 0xB000);
    uint8_t opcode = (instruction & 0x0080) >> 7;
    uint16_t offset = (instruction & 0x007F) << 2;

    switch (opcode) {
        case 0: {
            // 0: ADD  SP,#nn       ;SP = SP + nn
            uint32_t value = getRegister(SP_REGISTER) + offset;
            setRegister(SP_REGISTER, value);
            break;
        }
        case 1: {
            // 1: ADD  SP,#-nn      ;SP = SP - nn
            uint32_t value = getRegister(SP_REGISTER) - offset;
            setRegister(SP_REGISTER, value);
            break;
        }
    }
    return SEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::multipleLoadStoreHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0xC000);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    uint8_t rList = instruction & 0x00FF;
    uint8_t rb = (instruction & 0x0700) >> 8;
    uint32_t rbValue = getRegister(rb);
    uint32_t oldRbValue = rbValue;

    bool firstAccess = true;

    // In THUMB mode stack is always meant to be 'full descending', 
    // ie. PUSH is equivalent to 'STMFD/STMDB' and POP to 'LDMFD/LDMIA' in ARM mode.

    switch (opcode) {
        case 0: {
            // 0: STMIA Rb!,{Rlist}   ;store in memory, increments Rb (post-increment store)
            for(int i = 0; i < 8; i++) {
                if(rList & 0x01) {
                    if(firstAccess) {
                        bus->write32(rbValue, getRegister(i), Bus::CycleType::NONSEQUENTIAL);
                        firstAccess = false;
                    } else {
                        bus->write32(rbValue, getRegister(i), Bus::CycleType::SEQUENTIAL);
                    }
                    rbValue += 4;
                }
                rList >>= 1;
            }
            if(!(instruction & 0x00FF)) {
                // empty rList
                // R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5). 
                bus->write32(rbValue, getRegister(PC_REGISTER) + 4, Bus::CycleType::NONSEQUENTIAL);
                rbValue += 0x40;
            }
            if(((uint32_t)rList >> rb) & 0x1) {
                // if rb included in rList, Store OLD base if Rb is FIRST entry in Rlist, 
                // otherwise store NEW base (STM/ARMv4)
                if(!(rList << (8 - rb))) {
                    // rb is first entry in rList
                    setRegister(rb, oldRbValue);   
                } else {
                    setRegister(rb, rbValue);
                }
            } else {
                setRegister(rb, rbValue);
            }    
            break;
        }
        case 1: {
            // 1: LDMIA Rb!,{Rlist}   ;load from memory, increments Rb (post-increment load)
            for(int i = 0; i < 8; i++) {
                if(rList & 0x01) {
                    if(firstAccess) {
                        setRegister(i, bus->read32(rbValue, Bus::CycleType::NONSEQUENTIAL));
                        firstAccess = false;
                    } else {
                        setRegister(i, bus->read32(rbValue, Bus::CycleType::SEQUENTIAL));
                    }
                    rbValue += 4;
                }
                rList >>= 1;
            }
            if(!(instruction & 0x00FF)) {
                // empty rList
                // R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5).
                setRegister(PC_REGISTER, bus->read32(rbValue, Bus::CycleType::NONSEQUENTIAL));
                rbValue += 0x40;
            }
            if(((uint32_t)rList >> rb) & 0x1) {
                // if rb included in rList, no write back for stm
            } else {
                setRegister(rb, rbValue);
            }
            break;
        }
    }
    bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return NONSEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::multipleLoadStorePushPopHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0xB000);
    assert((instruction & 0x0600) == 0x0400);
    uint8_t opcode = (instruction & 0x0800) >> 11;
    bool pcLrBit = instruction & 0x0100; // 1: PUSH LR (R14), or POP PC (R15)
    uint8_t rList = instruction & 0x00FF;
    uint32_t spValue = getRegister(SP_REGISTER);
    bool firstAccess = true;
    bool branch = false;

    // In THUMB mode stack is always meant to be 'full descending', 
    // ie. PUSH is equivalent to 'STMFD/STMDB' and POP to 'LDMFD/LDMIA' in ARM mode.

    switch (opcode) {
        case 0: {
            // 0: PUSH {Rlist}{LR}   ;store in memory, decrements SP (R13)
            if(pcLrBit) {
                spValue -=4;
                // TODO: just realized! for push, or backwards block memory, 
                // the first data access in the for loop might be the last memory access in reality
                bus->write32(spValue, getRegister(LINK_REGISTER), Bus::CycleType::NONSEQUENTIAL);
                firstAccess = false;
            }
            for(int i = 7; i >= 0; i--) {
                if(rList & 0x80) {
                    spValue -= 4;
                    if(firstAccess) {
                        bus->write32(spValue, getRegister(i), Bus::CycleType::NONSEQUENTIAL);
                        firstAccess = false;
                    } else {
                        bus->write32(spValue, getRegister(i), Bus::CycleType::SEQUENTIAL);
                    }
                }
                rList <<= 1;
            }
            setRegister(SP_REGISTER, spValue);
            break;
        }
        case 1: {
            // 1: POP  {Rlist}{PC}   ;load from memory, increments SP (R13)
            for(int i = 0; i < 8; i++) {
                if(rList & 0x01) {
                    if(firstAccess) {
                        setRegister(i, bus->read32(spValue, Bus::CycleType::NONSEQUENTIAL));
                        firstAccess = false;
                    } else {
                        setRegister(i, bus->read32(spValue, Bus::CycleType::SEQUENTIAL));
                    }
                    spValue += 4;
                }
                rList >>= 1;
            }  

            if(pcLrBit) {
                // TODO, whether it's sequentiual or not might depend on whether rlist is empty
                setRegister(PC_REGISTER, bus->read32(spValue, Bus::CycleType::SEQUENTIAL) & 0xFFFFFFFE);
                spValue += 4;
            } 
            setRegister(SP_REGISTER, spValue);
            break;
        }
    }
    bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);

    if(!branch) {
        return BRANCH;
    } else {
        return NONSEQUENTIAL;
    }
}

static uint32_t signExtend9Bit(uint16_t value) {
    return (value & 0x0100) ? (((uint32_t)value) | 0xFFFFFE00) : value;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::conditionalBranchHandler(uint16_t instruction) {
    assert((instruction & 0xF000) == 0xD000);
    uint8_t opcode = (instruction & 0x0F00) >> 8;
    uint32_t offset = signExtend9Bit((instruction & 0x00FF) << 1);
    bool jump = false;

    // Destination address must by halfword aligned (ie. bit 0 cleared)
    // Return: No flags affected, PC adjusted if condition true
    switch (opcode) {
        case 0x0: {
            // 0: BEQ label        ;Z=1         ;equal (zero) (same)
            jump = cpsr.Z;
            break;
        }
        case 0x1: {
            // 1: BNE label        ;Z=0         ;not equal (nonzero) (not same)
            jump = !(cpsr.Z);
            break;
        }
        case 0x2: {
            // 2: BCS/BHS label    ;C=1         ;unsigned higher or same (carry set)
            jump = cpsr.C;
            break;
        }
        case 0x3: {
            // 3: BCC/BLO label    ;C=0         ;unsigned lower (carry cleared)
            jump = !(cpsr.C);
            break;
        }
        case 0x4: {
            // 4: BMI label        ;N=1         ;signed  negative (minus)
            jump = cpsr.N;
            break;
        }
        case 0x5: {
            // 5: BPL label        ;N=0         ;signed  positive or zero (plus)
            jump = !(cpsr.N);
            break;
        }
        case 0x6: {
            // 6: BVS label        ;V=1         ;signed  overflow (V set)
            jump = cpsr.V;
            break;
        }
        case 0x7: {
            // 7: BVC label        ;V=0         ;signed  no overflow (V cleared)
            jump = !(cpsr.V);
            break;
        }
        case 0x8: {
            // 8: BHI label        ;C=1 and Z=0 ;unsigned higher
            jump = !cpsr.Z && cpsr.C;
            break;
        }
        case 0x9: {
            // 9: BLS label        ;C=0 or Z=1  ;unsigned lower or same
            jump = !(cpsr.C) || cpsr.Z;
            break;
        }
        case 0xA: {
            // A: BGE label        ;N=V         ;signed greater or equal
            jump = cpsr.N == cpsr.V;
            break;
        }
        case 0xB: {
            // B: BLT label        ;N<>V        ;signed less than
            jump = cpsr.N != cpsr.V;
            break;
        }
        case 0xC: {
            // C: BGT label        ;Z=0 and N=V ;signed greater than
            jump = !(cpsr.Z) && (cpsr.N == cpsr.V);
            break;
        }
        case 0xD: {
            // D: BLE label        ;Z=1 or N<>V ;signed less or equal
            jump = (cpsr.Z) || (cpsr.N != cpsr.V);
            break;
        }
        case 0xE: {
            // E: Undefined, should not be used
            assert(false);
            break;
        }
        case 0xF: {
            //  F: Reserved for SWI instruction (see SWI opcode)
            assert(false);
            break;
        }
    }

    if(jump) {
        setRegister(PC_REGISTER, (getRegister(PC_REGISTER) + 2 + offset) & 0xFFFFFFFE);
        return BRANCH;
    } else {
        return SEQUENTIAL;
    }
}

static uint32_t signExtend12Bit(uint32_t value) {
    return (value & 0x00000800) ? (value | 0xFFFFF000) : value;
}

ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::unconditionalBranchHandler(uint16_t instruction) {
    assert((instruction & 0xF800) == 0xE000);
    uint32_t offset = signExtend12Bit((instruction & 0x07FF) << 1);
    setRegister(PC_REGISTER, (getRegister(PC_REGISTER) + 2 + offset) & 0xFFFFFFFE);

    return BRANCH; 
}

static uint32_t signExtend23Bit(uint32_t value) {
    return (value & 0x00400000) ? (value | 0xFF800000) : value;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::longBranchHandler(uint16_t instruction) {
    uint8_t opcode = (instruction & 0xF800) >> 11;

    switch(opcode) {
        case 0x1E: {
            // First Instruction - LR = PC+4+(nn SHL 12)
            assert((instruction & 0xF800) == 0xF000);
            uint32_t offsetHiBits = signExtend23Bit(((uint32_t)(instruction & 0x07FF)) << 12);            
            setRegister(LINK_REGISTER, getRegister(PC_REGISTER) + 2 + offsetHiBits);
            return SEQUENTIAL;
        }
        case 0x1F: {
            // Second Instruction - PC = LR + (nn SHL 1), and LR = PC+2 OR 1 (and BLX: T=0)
            // 11111b: BL label   ;branch long with link
            uint32_t offsetLoBits = ((uint32_t)(instruction & 0x07FF)) << 1;
            uint32_t temp = (getRegister(PC_REGISTER)) | 0x1;
            // The destination address range is (PC+4)-400000h..+3FFFFEh, 
            setRegister(PC_REGISTER, (getRegister(LINK_REGISTER) + offsetLoBits));
            setRegister(LINK_REGISTER, temp);  
            return BRANCH;
        }
        case 0x1D: {
            // 11101b: BLX label  ;branch long with link switch to ARM mode (ARM9)
            assert(false);
            DEBUGWARN("BLX not implemented!\n");
            break;
        }
    }

    return SEQUENTIAL;
}

inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::softwareInterruptHandler(uint16_t instruction) {
    // 11011111b: SWI nn   ;software interrupt
    // 10111110b: BKPT nn  ;software breakpoint (ARMv5 and up) not used in ARMv4T
    assert((instruction & 0xFF00) == 0xDF00);    

    // CPSR=<changed>  ;Enter svc/abt
    switchToMode(Mode::SUPERVISOR);

    // ARM state, IRQs disabled
    cpsr.T = 0;
    cpsr.I = 1;

    // R14_svc=PC+2    ;save return address
    // TODO explain why you arent adding 2
    setRegister(14, getRegister(PC_REGISTER));


    // PC=VVVV0008h    ;jump to SWI/PrefetchAbort vector address
    // TODO: is base always 0000?
    // TODO: make vector addresses static members
    setRegister(PC_REGISTER, 0x00000008);

    return BRANCH; 
}

/* ~~~~~~~~~~~~~~~ Undefined Operation ~~~~~~~~~~~~~~~~~~~~*/
inline
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::undefinedOpHandler(uint16_t instruction) {
    DEBUGWARN("UNDEFINED THUMB OPCODE! " << std::bitset<16>(instruction).to_string() << std::endl);
    // switchToMode(ARM7TDMI::Mode::UNDEFINED);
    // TODO: what is behaviour of thumb undefined instruction?
    bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return BRANCH;
}