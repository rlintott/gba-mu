#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"


// FFC0
template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbLdStRegOffHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0x5000);
    assert(!(instruction & 0x0200));
    //uint8_t opcode = (instruction & 0x0C00) >> 10;
    constexpr uint8_t opcode = (op & 0x030) >> 4;
    uint8_t rd = thumbGetRd(instruction);
    uint8_t rb = thumbGetRb(instruction);
    //uint8_t ro = (instruction & 0x01C0) >> 6;
    constexpr uint8_t ro = (op & 0x007);
    uint32_t address = cpu->getRegister(rb) + cpu->getRegister(ro);

    if constexpr(opcode == 0) {
        // 0: STR  Rd,[Rb,Ro]   ;store 32bit data  WORD[Rb+Ro] = Rd
        cpu->bus->write32(address, cpu->getRegister(rd), Bus::CycleType::NONSEQUENTIAL);
    } else if constexpr(opcode == 1) {
        // 1: STRB Rd,[Rb,Ro]   ;store  8bit data  BYTE[Rb+Ro] = Rd
        cpu->bus->write8(address, (uint8_t)(cpu->getRegister(rd)), Bus::CycleType::NONSEQUENTIAL);
    } else if constexpr(opcode == 2) {
        // 2: LDR  Rd,[Rb,Ro]   ;load  32bit data  Rd = WORD[Rb+Ro]
        uint32_t value = aluShiftRor(cpu->bus->read32(address, Bus::CycleType::NONSEQUENTIAL),
                                        (address & 3) * 8);
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);        
    } else {
        // opcode == 3
        // 3: LDRB Rd,[Rb,Ro]   ;load   8bit data  Rd = BYTE[Rb+Ro]
        uint32_t value = (uint32_t)cpu->bus->read8(address, Bus::CycleType::NONSEQUENTIAL);
        cpu->setRegister(rd, value);
        cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    }

    return NONSEQUENTIAL;
}
