
#pragma once
#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"



template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armHwdtHandler(uint32_t instruction, ARM7TDMI* cpu) {

    constexpr bool l = op & 0x010;
    constexpr bool p = op & 0x100;
    constexpr bool u = op & 0x080;
    constexpr bool w = op & 0x020;
    constexpr bool i = op & 0x040;
    constexpr uint8_t opcode = (op & 0x006) >> 1;

    assert((instruction & 0x0E000000) == 0);

    uint8_t rd = getRd(instruction);
    uint32_t rdVal =
        (rd == 15) ? cpu->getRegister(rd) + 8 : cpu->getRegister(rd);
    uint8_t rn = getRn(instruction);
    uint32_t rnVal =
        (rn == 15) ? cpu->getRegister(rn) + 4 : cpu->getRegister(rn);

    uint32_t offset = 0;

    if constexpr(i) {
        // immediate as offset
        offset = (((instruction & 0x00000F00) >> 4) | (instruction & 0x0000000F));
    } else {
        // register as offset
        assert(!(instruction & 0x00000F00));
        assert(getRm(instruction) != 15);
        offset = cpu->getRegister(getRm(instruction));
    }
    assert(instruction & 0x00000080);
    assert(instruction & 0x00000010);

    uint32_t address = rnVal;
    if constexpr(p) {
        // pre-indexing offset
        if constexpr(u) {
            address = address + offset;
        } else {
            address = address - offset;
        }
        if constexpr(w) {
            // write address back into base register
            cpu->setRegister(rn, address);
        }
    } else {
        // post-indexing offset
        assert(dataTransGetW(instruction) == 0);
        // add offset after transfer and always write back
        if constexpr(u) {
            cpu->setRegister(rn, address + offset);
        } else {
            cpu->setRegister(rn, address - offset);
        }
    }

    if constexpr(opcode  == 0) {
        // Reserved for SWP instruction
        assert(false);
    } else if constexpr(opcode == 1) { // STRH or LDRH (depending on l)
        if constexpr(l) {  // LDR{cond}H  Rd,<Address>  ;Load Unsigned halfword
                    // (zero-extended)         
            cpu->setRegister(rd, aluShiftRor(
                                        (uint32_t)(cpu->bus->read16(address, Bus::CycleType::NONSEQUENTIAL)),
                                        (address & 1) * 8));
            cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
        } else {  // STR{cond}H  Rd,<Address>  ;Store halfword   [a]=Rd
            cpu->bus->write16(address, (uint16_t)rdVal, Bus::CycleType::NONSEQUENTIAL);
        }

    } else if constexpr(opcode == 2) {// LDR{cond}SB Rd,<Address>  ;Load Signed byte (sign extended)
        // TODO: better way to do this?
        assert(l);
        uint32_t val = (uint32_t)(cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL));
        if (val & 0x00000080) {
            cpu->setRegister(rd, 0xFFFFFF00 | val);
        } else {
            cpu->setRegister(rd, val);
        }
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    } else {
        // opcode == 3
        // LDR{cond}SH Rd,<Address>  ;Load Signed halfword (sign extended)
        if(address & 0x00000001) {
            // TODO refactor this, reusing the same code as case 2
            // strange case: LDRSH Rd,[odd]  -->  LDRSB Rd,[odd] ;sign-expand BYTE value
            assert(dataTransGetL(instruction));
            uint32_t val = (uint32_t)(cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL));
            if (val & 0x00000080) {
                cpu->setRegister(rd, 0xFFFFFF00 | val);
            } else {
                cpu->setRegister(rd, val);
            }
        } else {
            assert(l);
            uint32_t val = (uint32_t)(cpu->bus->read16(address, Bus::CycleType::NONSEQUENTIAL));
            if (val & 0x00008000) {
                cpu->setRegister(rd, 0xFFFF0000 | val);
            } else {
                cpu->setRegister(rd, val);
            }
            cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
        }
    }
    if constexpr(l) {
        if(rd == PC_REGISTER) {
            // LDR PC
            return BRANCH;
        } else {
            return NONSEQUENTIAL;
        }
    } else {
        return NONSEQUENTIAL;
    }

}