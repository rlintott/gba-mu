#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

// op = instruction & 0xFFC0;
// anything within the mask can be constexpr!

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbAluHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xFC00) == 0x4000);
    //uint8_t opcode = (instruction & 0x03C0) >> 6;
    constexpr uint8_t opcode = (op & 0x00F);

    uint8_t rs = thumbGetRs(instruction);
    uint8_t rd = thumbGetRd(instruction);
    bool carryFlag, overflowFlag, signFlag, zeroFlag;

    Cycles cycles;
    int internalCycles = 0;


    if constexpr(opcode == 0x0) {
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
    } else if constexpr(opcode == 0x1) {
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
    } else if constexpr(opcode == 0x2) {
        // 2: LSL{S} Rd,Rs     ;log. shift left   Rd = Rd << (Rs AND 0FFh)
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint8_t offset = rsVal & 0xFF;
        uint32_t result;
        result = aluShiftLsl(rdVal, offset);
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        // TODO:
        if(!offset) {
            carryFlag = cpu->cpsr.C;
        } else {
            carryFlag = aluLslSetsCarryBit(rdVal, offset);
        }
        overflowFlag = cpu->cpsr.V;
        cpu->setRegister(rd, result);
        internalCycles = 1;
    } else if constexpr(opcode == 0x3) {
        // 3: LSR{S} Rd,Rs     ;log. shift right  Rd = Rd >> (Rs AND 0FFh)
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint8_t offset = rsVal & 0xFF;
        uint32_t result;
        result = aluShiftLsr(rdVal, offset);
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        // TODO:
        carryFlag = !(offset) ? cpu->cpsr.C : aluLsrSetsCarryBit(rdVal, offset);
        overflowFlag = cpu->cpsr.V;
        cpu->setRegister(rd, result);
        internalCycles = 1;
    } else if constexpr(opcode == 0x4) {
        // 4: ASR{S} Rd,Rs     ;arit shift right  Rd = Rd SAR (Rs AND 0FFh)
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint8_t offset = rsVal & 0xFF;
        uint32_t result;
        result = aluShiftAsr(rdVal, offset);
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        // TODO:
        carryFlag = !(offset) ? (cpu->cpsr.C) : aluAsrSetsCarryBit(rdVal, offset);
        overflowFlag = cpu->cpsr.V;
        cpu->setRegister(rd, result);
        internalCycles = 1;        
    } else if constexpr(opcode == 0x5) {
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
    } else if constexpr(opcode == 0x6) {
            // 6: SBC{S} Rd,Rs     ;sub with carry    Rd = Rd - Rs - NOT Cy
        uint64_t rsVal = cpu->getRegister(rs);
        uint64_t rdVal = cpu->getRegister(rd);
        uint64_t result;
        result = rdVal - rsVal - !(uint64_t)(cpu->cpsr.C);
        signFlag = aluSetsSignBit((uint32_t)result);
        zeroFlag = aluSetsZeroBit((uint32_t)result);
        carryFlag = aluSubWithCarrySetsCarryBit(result);
        overflowFlag = aluSubWithCarrySetsOverflowBit(rdVal, rsVal, result, cpu);
        cpu->setRegister(rd, result);  
    } else if constexpr(opcode == 0x7) {
        // 7: ROR{S} Rd,Rs     ;rotate right      Rd = Rd ROR (Rs AND 0FFh)
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint8_t offset = rsVal & 0xFF;
        uint32_t result;
        result = aluShiftRor(rdVal, offset % 32);
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        overflowFlag = cpu->cpsr.V;
        // TODO:
        carryFlag = !(offset) ? (cpu->cpsr.C) : aluRorSetsCarryBit(rdVal, offset);
        cpu->setRegister(rd, result);
        internalCycles = 1;        
    } else if constexpr(opcode == 0x8) {
        // 8: TST    Rd,Rs     ;test            Void = Rd AND Rs
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result;
        result = rdVal & rsVal;
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        carryFlag = cpu->cpsr.C;
        overflowFlag = cpu->cpsr.V;        
    } else if constexpr(opcode == 0x9) {
        // 9: NEG{S} Rd,Rs     ;negate            Rd = 0 - Rs
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t result;
        result = (uint32_t)0 - rsVal;
        signFlag = aluSetsSignBit((uint32_t)result);
        zeroFlag = aluSetsZeroBit((uint32_t)result);
        carryFlag = aluSubtractSetsCarryBit(0, rsVal);
        overflowFlag = aluSubtractSetsOverflowBit(0, rsVal, result);
        cpu->setRegister(rd, result);       
    } else if constexpr(opcode == 0xA) {
        // A: CMP    Rd,Rs     ;compare         Void = Rd - Rs
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result;
        result = rdVal - rsVal;
        signFlag = aluSetsSignBit((uint32_t)result);
        zeroFlag = aluSetsZeroBit((uint32_t)result);
        carryFlag = aluSubtractSetsCarryBit(rdVal, rsVal);
        overflowFlag = aluSubtractSetsOverflowBit(rdVal, rsVal, result);        
    } else if constexpr(opcode == 0xB) {
        // B: CMN    Rd,Rs     ;neg.compare     Void = Rd + Rs
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result;
        result = rdVal + rsVal;
        signFlag = aluSetsSignBit((uint32_t)result);
        zeroFlag = aluSetsZeroBit((uint32_t)result);
        carryFlag = aluAddSetsCarryBit(rdVal, rsVal);
        overflowFlag = aluAddSetsOverflowBit(rdVal, rsVal, result);        
    } else if constexpr(opcode == 0xC) {
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
    } else if constexpr(opcode == 0xD) {
        // D: MUL{S} Rd,Rs     ;multiply          Rd = Rd * Rs
        uint64_t rsVal = cpu->getRegister(rs);
        uint64_t rdVal = cpu->getRegister(rd);
        uint64_t result;
        result = rdVal * rsVal;
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        // carry flag destroyed
        // TODO: why? it says on GBATek ARMv4 and below: carry flag destroyed
        carryFlag = cpu->cpsr.C;
        // carryFlag = 0;
        overflowFlag = cpu->cpsr.V;
        cpu->setRegister(rd, result);
        internalCycles = mulGetExecutionTimeMVal(rdVal);  
    } else if constexpr(opcode == 0xE) {
        // E: BIC{S} Rd,Rs     ;bit clear         Rd = Rd AND NOT Rs

        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t rdVal = cpu->getRegister(rd);
        uint32_t result;
        result = rdVal & (~rsVal);
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        carryFlag = (cpu->cpsr.C);
        overflowFlag = cpu->cpsr.V;
        cpu->setRegister(rd, result);   
    } else {
        // opcode == 0xF
        // F: MVN{S} Rd,Rs     ;not               Rd = NOT Rs
        uint32_t rsVal = cpu->getRegister(rs);
        uint32_t result;
        result = ~rsVal;
        signFlag = aluSetsSignBit(result);
        zeroFlag = aluSetsZeroBit(result);
        carryFlag = cpu->cpsr.C;
        overflowFlag = cpu->cpsr.V;
        cpu->setRegister(rd, result);
    }

    cpu->cpsr.N = signFlag;
    cpu->cpsr.Z = zeroFlag;
    cpu->cpsr.C = carryFlag;
    cpu->cpsr.V = overflowFlag;

    for(int i = 0; i < internalCycles; i++) {
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }
    if(!internalCycles) {
        return SEQUENTIAL;
    } else {
        return NONSEQUENTIAL;
    }
}
