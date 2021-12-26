
#pragma once
#include "../ARM7TDMI.h"
#include "../memory/Bus.h"


template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armSdtHandler(uint32_t instruction, ARM7TDMI* cpu) {
    constexpr uint8_t opcode = (op & 0x006) >> 1;
    constexpr bool p = op & 0x100;
    constexpr bool u = op & 0x080;
    constexpr bool b = op & 0x040;
    constexpr bool l = op & 0x010;
    constexpr bool w = op & 0x020;
    constexpr bool i = op & 0x200;

    // TODO:  implement the following restriction <expression>  ;an immediate
    // used as address
    // ;*** restriction: must be located in range PC+/-4095+8, if so,
    // ;*** assembler will calculate offset and use PC (R15) as base.
    assert((instruction & 0x0C000000) == (instruction & 0x04000000));
    uint8_t rd = getRd(instruction);
    uint32_t rdVal = (rd == 15) ? cpu->getRegister(rd) + 8 : cpu->getRegister(rd);
    uint8_t rn = getRn(instruction);
    uint32_t rnVal = (rn == 15) ? cpu->getRegister(rn) + 4 : cpu->getRegister(rn);

    uint32_t offset;
    // I - Immediate Offset Flag (0=Immediate, 1=Shifted Register)
    if constexpr(i) {
        // Register shifted by Immediate as Offset
        assert(!(instruction & 0x00000010));  // bit 4 Must be 0 (Reserved, see
                                              // The Undefined Instruction)
        uint8_t rm = getRm(instruction);
        assert(rm != 15);
        uint8_t shiftAmount = (instruction & 0x00000F80) >> 7;

        if constexpr(opcode == 0) {
            offset = shiftAmount != 0
                            ? aluShiftLsl(cpu->getRegister(rm), shiftAmount)
                            : cpu->getRegister(rm);
        } else if constexpr(opcode == 1) {
            offset = shiftAmount != 0
                            ? aluShiftLsr(cpu->getRegister(rm), shiftAmount)
                            : 0;

        } else if constexpr(opcode == 2) {
            offset = shiftAmount != 0
                            ? aluShiftAsr(cpu->getRegister(rm), shiftAmount)
                            : aluShiftAsr(cpu->getRegister(rm), 32);
        } else {
            // opcode == 3
            offset = (shiftAmount != 0
                ? aluShiftRor(cpu->getRegister(rm), shiftAmount % 32)
                : aluShiftRrx(cpu->getRegister(rm), 1, cpu));
        }
    } else {  // immediate as offset (12 bit offset)
        offset = instruction & 0x00000FFF;
    }
    uint32_t address = rnVal;

    // U - Up/Down Bit (0=down; subtract offset
    // from base, 1=up; add to base)

    // P - Pre/Post (0=post; add offset after
    // transfer, 1=pre; before trans.)

    if constexpr(p) {  // add offset before transfer
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
        // add offset after transfer and always write back
        if constexpr(u) {
            cpu->setRegister(rn, address + offset);
        } else {    
            cpu->setRegister(rn, address - offset);
        }
    }

    // B - Byte/Word bit (0=transfer
    // 32bit/word, 1=transfer 8bit/byte)
    // TODO implement t bit, force non-privilege access
    // L - Load/Store bit (0=Store to memory, 1=Load from memory)
    if constexpr(l) {
        // LDR{cond}{B}{T} Rd,<Address> ;Rd=[Rn+/-<offset>]
        if constexpr(b) {  // transfer 8 bits
            cpu->setRegister(rd, (uint32_t)(cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL)));
        } else {  // transfer 32 bits
            // aligned to word
            // Reads from forcibly aligned address "addr AND (NOT 3)",
            // and does then rotate the data as "ROR (addr AND 3)*8". T
            // TODO: move the masking and shifting into the read/write functions
            cpu->setRegister(rd, aluShiftRor(cpu->bus->read32(address,
                                Bus::CycleType::NONSEQUENTIAL),
                                (address & 3) * 8));
        }
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
        if(rd == PC_REGISTER) {
            return BRANCH;
        } else {
            return NONSEQUENTIAL;
        }
    } else {
        // STR{cond}{B}{T} Rd,<Address>   ;[Rn+/-<offset>]=Rd
        if constexpr(b) {  // transfer 8 bits
            cpu->bus->write8(address, (uint8_t)(rdVal), 
                             Bus::CycleType::NONSEQUENTIAL);
        } else {  // transfer 32 bits
            cpu->bus->write32(address, (rdVal), 
                              Bus::CycleType::NONSEQUENTIAL);
        }
        
        return NONSEQUENTIAL;
    }
}