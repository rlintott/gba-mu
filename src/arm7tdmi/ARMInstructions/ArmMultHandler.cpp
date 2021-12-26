#include "../../memory/Bus.h"

template<uint16_t op> 
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armMultHandler(uint32_t instruction, ARM7TDMI* cpu) {

    constexpr uint8_t opcode = (op & 0x1E0) >> 5;
    constexpr bool s = (op & 0x010);
    // rd is different for multiply
    uint8_t rd = (instruction & 0x000F0000) >> 16;
    uint8_t rm = getRm(instruction);
    uint8_t rs = getRs(instruction);
    assert(rd != rm && (rd != PC_REGISTER && rm != PC_REGISTER && rs != PC_REGISTER));
    assert((instruction & 0x000000F0) == 0x00000090);
    uint64_t result;
    BitPreservedInt64 longResult;
    int internalCycles;

    if constexpr(opcode == 0b0000) {// MUL
        uint32_t rsVal = cpu->getRegister(rs);
        result = (uint64_t)cpu->getRegister(rm) * (uint64_t)rsVal;
        cpu->setRegister(rd, (uint32_t)result);
        internalCycles = mulGetExecutionTimeMVal(rsVal);
    } else if constexpr(opcode == 0b0001) {  // MLA
        // rn is different for multiply
        uint8_t rn = (instruction & 0x0000F000) >> 12;
        uint32_t rsVal = cpu->getRegister(rs);
        assert(rn != PC_REGISTER);
        result = (uint64_t)cpu->getRegister(rm) *
                    (uint64_t)rsVal +
                    (uint64_t)cpu->getRegister(rn);
        cpu->setRegister(rd, (uint32_t)result);
        internalCycles = mulGetExecutionTimeMVal(rsVal) + 1;
    } else if constexpr(opcode ==  0b0100) {  // UMULL{cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs
        uint32_t rsVal = cpu->getRegister(rs);
        uint8_t rdhi = rd;
        uint8_t rdlo = (instruction & 0x0000F000) >> 12;
        longResult._unsigned = (uint64_t)cpu->getRegister(rm) * (uint64_t)rsVal;
        // high destination reg
        cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >> 32));
        cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
        // TODO: this is possible quite inefficient
        internalCycles = umullGetExecutionTimeMVal(rsVal) + 1;
    } else if constexpr(opcode == 0b0101) {  // UMLAL {cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs+RdHiLo
        uint32_t rsVal = cpu->getRegister(rs);
        uint8_t rdhi = rd;
        uint8_t rdlo = (instruction & 0x0000F000) >> 12;
        longResult._unsigned =
            (uint64_t)cpu->getRegister(rm) *
            (uint64_t)cpu->getRegister(rs) +
            ((((uint64_t)(cpu->getRegister(rdhi))) << 32) | ((uint64_t)(cpu->getRegister(rdlo))));
        // high destination reg
        cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >> 32));
        cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
        internalCycles = umullGetExecutionTimeMVal(rsVal) + 2;
    } else if constexpr(opcode == 0b0110) {  // SMULL {cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs
        uint32_t rsValRaw = cpu->getRegister(rs);
        uint8_t rdhi = rd;
        uint8_t rdlo = (instruction & 0x0000F000) >> 12;
        BitPreservedInt32 rmVal;
        BitPreservedInt32 rsVal;
        rmVal._unsigned = cpu->getRegister(rm);
        rsVal._unsigned = rsValRaw;
        longResult._signed =
            (int64_t)rmVal._signed * (int64_t)rsVal._signed;
        // high destination reg
        cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >> 32));
        cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
        internalCycles = mulGetExecutionTimeMVal(rsValRaw) + 1;
    } else if constexpr(opcode == 0b0111) {  // SMLAL{cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs+RdHiLo
        uint32_t rsValRaw = cpu->getRegister(rs);
        uint8_t rdhi = rd;
        uint8_t rdlo = (instruction & 0x0000F000) >> 12;
        BitPreservedInt32 rmVal;
        BitPreservedInt32 rsVal;
        rmVal._unsigned = cpu->getRegister(rm);
        rsVal._unsigned = rsValRaw;
        BitPreservedInt64 accum;
        accum._unsigned = ((((uint64_t)(cpu->getRegister(rdhi))) << 32) |
                            ((uint64_t)(cpu->getRegister(rdlo))));
        longResult._signed = (int64_t)rmVal._signed * (int64_t)rsVal._signed + accum._signed;
        // high destination reg
        cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >> 32));
        cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
        internalCycles = mulGetExecutionTimeMVal(rsValRaw) + 2;
    }
    
  if constexpr(s) {
        if constexpr(!(opcode & 0b0100)) {  // regular mult opcode,
            cpu->cpsr.Z = aluSetsZeroBit((uint32_t)result);
            cpu->cpsr.N = aluSetsSignBit((uint32_t)result);
        } else {
            cpu->cpsr.Z = (longResult._unsigned == 0);
            cpu->cpsr.N = (longResult._unsigned >> 63);
        }
        // cpsr.C = 0;
        // cpsr.V = 0;
    }
    // TODO: can probably optimize this greatly
    for(int i = 0; i < internalCycles; i++) {
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }
    if(internalCycles == 0) {
        return SEQUENTIAL;
    } else {
        return NONSEQUENTIAL;
    }
}