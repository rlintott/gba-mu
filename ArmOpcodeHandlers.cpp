#include <bitset>

#include "ARM7TDMI.h"

ARM7TDMI::Cycles ARM7TDMI::ArmOpcodeHandlers::multiplyHandler(
    uint32_t instruction, ARM7TDMI *cpu) {
    uint8_t opcode = getOpcode(instruction);
    uint8_t rd =
        (instruction & 0x000F0000) >> 16;  // rd is different for multiply
    uint8_t rm = getRm(instruction);
    uint8_t rs = getRs(instruction);
    assert(rd != rm &&
           (rd != PC_REGISTER && rm != PC_REGISTER && rs != PC_REGISTER));
    assert((instruction & 0x000000F0) == 0x00000090);
    uint64_t result;
    BitPreservedInt64 longResult;

    switch (opcode) {
        case 0b0000: {  // MUL
            result =
                (uint64_t)cpu->getRegister(rm) * (uint64_t)cpu->getRegister(rs);
            cpu->setRegister(rd, (uint32_t)result);
            break;
        }
        case 0b0001: {  // MLA
            uint8_t rn = (instruction & 0x0000F000) >>
                         12;  // rn is different for multiply
            assert(rn != PC_REGISTER);
            result = (uint64_t)cpu->getRegister(rm) *
                         (uint64_t)cpu->getRegister(rs) +
                     (uint64_t)cpu->getRegister(rn);
            cpu->setRegister(rd, (uint32_t)result);
            break;
        }
        case 0b0100: {  // UMULL{cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs
            uint8_t rdhi = rd;
            uint8_t rdlo = (instruction & 0x0000F000) >> 12;
            longResult._unsigned =
                (uint64_t)cpu->getRegister(rm) * (uint64_t)cpu->getRegister(rs);
            cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >>
                                              32));  // high destination reg
            cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
            break;
        }
        case 0b0101: {  // UMLAL {cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs+RdHiLo
            uint8_t rdhi = rd;
            uint8_t rdlo = (instruction & 0x0000F000) >> 12;
            longResult._unsigned =
                (uint64_t)cpu->getRegister(rm) *
                    (uint64_t)cpu->getRegister(rs) +
                ((((uint64_t)(cpu->getRegister(rdhi))) << 32) |
                 ((uint64_t)(cpu->getRegister(rdlo))));
            cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >>
                                              32));  // high destination reg
            cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
            break;
        }
        case 0b0110: {  // SMULL {cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs
            uint8_t rdhi = rd;
            uint8_t rdlo = (instruction & 0x0000F000) >> 12;
            BitPreservedInt32 rmVal;
            BitPreservedInt32 rsVal;
            rmVal._unsigned = cpu->getRegister(rm);
            rsVal._unsigned = cpu->getRegister(rs);
            longResult._signed =
                (int64_t)rmVal._signed * (int64_t)rsVal._signed;
            cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >>
                                              32));  // high destination reg
            cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
            break;
        }
        case 0b0111: {  // SMLAL{cond}{S} RdLo,RdHi,Rm,Rs ;RdHiLo=Rm*Rs+RdHiLo
            uint8_t rdhi = rd;
            uint8_t rdlo = (instruction & 0x0000F000) >> 12;
            BitPreservedInt32 rmVal;
            BitPreservedInt32 rsVal;
            rmVal._unsigned = cpu->getRegister(rm);
            rsVal._unsigned = cpu->getRegister(rs);
            BitPreservedInt64 accum;
            accum._unsigned = ((((uint64_t)(cpu->getRegister(rdhi))) << 32) |
                               ((uint64_t)(cpu->getRegister(rdlo))));
            longResult._signed =
                (int64_t)rmVal._signed * (int64_t)rsVal._signed + accum._signed;
            cpu->setRegister(rdhi, (uint32_t)(longResult._unsigned >>
                                              32));  // high destination reg
            cpu->setRegister(rdlo, (uint32_t)longResult._unsigned);
            break;
        }
    }

    if (sFlagSet(instruction)) {
        if (!(opcode & 0b0100)) {  // regular mult opcode,
            cpu->cpsr.Z = aluSetsZeroBit((uint32_t)result);
            cpu->cpsr.N = aluSetsSignBit((uint32_t)result);
        } else {
            cpu->cpsr.Z = (longResult._unsigned == 0);
            cpu->cpsr.N = (longResult._unsigned >> 63);
        }
        cpu->cpsr.C = 0;
    }
    return {};
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PSR Transfer (MRS, MSR) Operations
 * ~~~~~~~~~~~~~~~~~~~~*/

ARM7TDMI::Cycles ARM7TDMI::ArmOpcodeHandlers::psrHandler(uint32_t instruction,
                                                         ARM7TDMI *cpu) {
    assert(!(instruction & 0x0C000000));
    assert(!sFlagSet(instruction));

    bool immediate =
        (instruction & 0x02000000);  // bit 25: I - Immediate Operand Flag
                                     // (0=Register, 1=Immediate) (Zero for MRS)
    bool psrSource =
        (instruction & 0x00400000);  // bit 22: Psr - Source/Destination PSR
                                     // (0=CPSR, 1=SPSR_<current mode>)

    switch ((instruction & 0x00200000) >>
            21) {  // bit 21: special opcode for PSR
        case 0: {  // MRS{cond} Rd,Psr ; Rd = Psr
            assert(!immediate);
            assert(getRn(instruction) == 0xF);
            assert(!(instruction & 0x00000FFF));
            uint8_t rd = getRd(instruction);
            if (psrSource) {
                cpu->setRegister(rd, psrToInt(cpu->cpsr));
            } else {
                cpu->setRegister(rd, psrToInt(*(cpu->getCurrentModeSpsr())));
            }
            break;
        }
        case 1: {  // MSR{cond} Psr{_field},Op  ;Psr[field] = Op=
            assert((instruction & 0x0000F000) == 0x0000F000);
            uint8_t fscx = (instruction & 0x000F0000) >> 16;
            ProgramStatusRegister *psr =
                (psrSource ? &(cpu->cpsr) : cpu->getCurrentModeSpsr());
            if (immediate) {
                uint32_t immValue = (uint32_t)(instruction & 0x000000FF);
                uint8_t shift = (instruction & 0x00000F00) >> 7;
                cpu->transferToPsr(aluShiftRor(immValue, shift), fscx, psr);
            } else {  // register
                assert(!(instruction & 0x00000FF0));
                assert(getRm(instruction) != PC_REGISTER);
                cpu->transferToPsr(cpu->getRegister(getRm(instruction)), fscx,
                                   psr);
            }
            break;
        }
    }

    return {};
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALU OPERATIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// TODO: put assertions for every unexpected or unallowed circumstance (for
// deubgging)
// TODO: cycle calculation
/*
    0: AND{cond}{S} Rd,Rn,Op2    ;AND logical       Rd = Rn AND Op2
    1: EOR{cond}{S} Rd,Rn,Op2    ;XOR logical       Rd = Rn XOR Op2
    2: SUB{cond}{S} Rd,Rn,Op2 ;* ;subtract          Rd = Rn-Op2
    3: RSB{cond}{S} Rd,Rn,Op2 ;* ;subtract reversed Rd = Op2-Rn
    4: ADD{cond}{S} Rd,Rn,Op2 ;* ;add               Rd = Rn+Op2
    5: ADC{cond}{S} Rd,Rn,Op2 ;* ;add with carry    Rd = Rn+Op2+Cy
    6: SBC{cond}{S} Rd,Rn,Op2 ;* ;sub with carry    Rd = Rn-Op2+Cy-1
    7: RSC{cond}{S} Rd,Rn,Op2 ;* ;sub cy. reversed  Rd = Op2-Rn+Cy-1
    8: TST{cond}{P}    Rn,Op2    ;test            Void = Rn AND Op2
    9: TEQ{cond}{P}    Rn,Op2    ;test exclusive  Void = Rn XOR Op2
    A: CMP{cond}{P}    Rn,Op2 ;* ;compare         Void = Rn-Op2
    B: CMN{cond}{P}    Rn,Op2 ;* ;compare neg.    Void = Rn+Op2
    C: ORR{cond}{S} Rd,Rn,Op2    ;OR logical        Rd = Rn OR Op2
    D: MOV{cond}{S} Rd,Op2       ;move              Rd = Op2
    E: BIC{cond}{S} Rd,Rn,Op2    ;bit clear         Rd = Rn AND NOT Op2
    F: MVN{cond}{S} Rd,Op2       ;not               Rd = NOT Op2
*/

ARM7TDMI::Cycles ARM7TDMI::ArmOpcodeHandlers::aluHandler(uint32_t instruction,
                                                         ARM7TDMI *cpu) {
    // shift op2
    AluShiftResult shiftResult = cpu->aluShift(
        instruction, (instruction & 0x02000000), (instruction & 0x00000010));
    uint8_t rd = getRd(instruction);
    uint8_t rn = getRn(instruction);
    uint8_t opcode = getOpcode(instruction);
    bool carryBit = (cpu->cpsr).C;
    bool overflowBit = (cpu->cpsr).V;
    bool signBit = (cpu->cpsr).N;
    bool zeroBit = (cpu->cpsr).Z;

    uint32_t rnValue;
    // if rn == pc regiser, have to add to it to account for pipelining /
    // prefetching
    // TODO probably dont need this logic if pipelining is emulated
    if (rn != PC_REGISTER) {
        rnValue = cpu->getRegister(rn);
    } else if (!(instruction & 0x02000000) && (instruction & 0x00000010)) {
        rnValue = cpu->getRegister(rn) + 12;
    } else {
        rnValue = cpu->getRegister(rn) + 8;
    }
    uint32_t op2 = shiftResult.op2;

    uint32_t rnVal = cpu->getRegister(rn);

    switch (opcode) {
        case AND: {  // AND
            DEBUG("AND" << std::endl);
            uint32_t result = rnVal & op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case EOR: {  // EOR
            DEBUG("EOR" << std::endl);
            uint32_t result = rnVal ^ op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case SUB: {  // SUB
            DEBUG("SUB" << std::endl);
            uint32_t result = rnVal - op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            carryBit = aluSubtractSetsCarryBit(rnVal, op2);
            overflowBit = aluSubtractSetsOverflowBit(rnVal, op2, result);
            break;
        }
        case RSB: {  // RSB
            DEBUG("RSB" << std::endl);
            uint32_t result = op2 - rnVal;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            carryBit = aluSubtractSetsCarryBit(op2, rnVal);
            overflowBit = aluSubtractSetsOverflowBit(op2, rnVal, result);
            break;
        }
        case ADD: {  // ADD
            DEBUG("ADD" << std::endl);
            uint32_t result = rnVal + op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            carryBit = aluAddSetsCarryBit(rnVal, op2);
            overflowBit = aluAddSetsOverflowBit(rnVal, op2, result);
            break;
        }
        case ADC: {  // ADC
            DEBUG("ADC" << std::endl);
            uint64_t result = (uint64_t)(rnVal + op2 + (cpu->cpsr).C);
            cpu->setRegister(rd, (uint32_t)result);
            zeroBit = aluSetsZeroBit((uint32_t)result);
            signBit = aluSetsSignBit((uint32_t)result);
            carryBit = aluAddWithCarrySetsCarryBit(result);
            overflowBit = aluAddWithCarrySetsOverflowBit(rnVal, op2,
                                                         (uint32_t)result, cpu);
            break;
        }
        case SBC: {  // SBC
            DEBUG("SBC" << std::endl);
            uint64_t result = rnVal + (~op2) + (cpu->cpsr).C;
            cpu->setRegister(rd, (uint32_t)result);
            zeroBit = aluSetsZeroBit((uint32_t)result);
            signBit = aluSetsSignBit((uint32_t)result);
            carryBit = aluSubWithCarrySetsCarryBit(result);
            overflowBit = aluSubWithCarrySetsOverflowBit(rnVal, op2,
                                                         (uint32_t)result, cpu);
            break;
        }
        case RSC: {  // RSC
            DEBUG("RSC" << std::endl);
            uint64_t result = op2 + (~rnVal) + (cpu->cpsr).C;
            cpu->setRegister(rd, (uint32_t)result);
            zeroBit = aluSetsZeroBit((uint32_t)result);
            signBit = aluSetsSignBit((uint32_t)result);
            carryBit = aluSubWithCarrySetsCarryBit(result);
            overflowBit = aluSubWithCarrySetsOverflowBit(op2, rnVal,
                                                         (uint32_t)result, cpu);
            break;
        }
        case TST: {  // TST
            DEBUG("TEST" << std::endl);
            uint32_t result = rnVal & op2;
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case TEQ: {  // TEQ
            DEBUG("TEQ" << std::endl);
            uint32_t result = rnVal ^ op2;
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case CMP: {  // CMP
            DEBUG("CMP" << std::endl);
            uint32_t result = rnVal - op2;
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            carryBit = aluSubtractSetsCarryBit(rnVal, op2);
            overflowBit = aluSubtractSetsOverflowBit(rnVal, op2, result);
            break;
        }
        case CMN: {  // CMN
            DEBUG("CMN" << std::endl);
            uint32_t result = rnVal + op2;
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            carryBit = aluAddSetsCarryBit(rnVal, op2);
            overflowBit = aluAddSetsOverflowBit(rnVal, op2, result);
            break;
        }
        case ORR: {  // ORR
            DEBUG("ORR" << std::endl);
            uint32_t result = rnVal | op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case MOV: {  // MOV
            DEBUG("MOV" << std::endl);
            uint32_t result = op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case BIC: {  // BIC
            DEBUG("BIC" << std::endl);
            uint32_t result = rnVal & (~op2);
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
        case MVN: {  // MVN
            DEBUG("MVN" << std::endl);
            uint32_t result = ~op2;
            cpu->setRegister(rd, result);
            zeroBit = aluSetsZeroBit(result);
            signBit = aluSetsSignBit(result);
            break;
        }
    }

    if (rd != PC_REGISTER && sFlagSet(instruction)) {
        cpu->cpsr.C = carryBit;
        cpu->cpsr.Z = zeroBit;
        cpu->cpsr.N = signBit;
        cpu->cpsr.V = overflowBit;
    } else if (rd == PC_REGISTER && sFlagSet(instruction)) {
        cpu->cpsr = *(cpu->getCurrentModeSpsr());
    } else {
    }  // flags not affected, not allowed in CMP

    return {};
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Undefined Operation
 * ~~~~~~~~~~~~~~~~~~~~*/

ARM7TDMI::Cycles ARM7TDMI::ArmOpcodeHandlers::undefinedOpHandler(
    uint32_t instruction, ARM7TDMI *cpu) {
    DEBUG("UNDEFINED OPCODE! " << std::bitset<32>(instruction).to_string()
                               << std::endl);
    return {};
}
