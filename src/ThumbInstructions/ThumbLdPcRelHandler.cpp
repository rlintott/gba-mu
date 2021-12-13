#include "../ARM7TDMI.h"
#include "../Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbLdPcRelHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF800) == 0x4800);
    DEBUG("in THUMB.6: load PC-relative \n");
    uint16_t offset = (instruction & 0x00FF) << 2;
    // uint8_t rd = (instruction & 0x0700) >> 8;
    //01 1100
    constexpr uint8_t rd = (op & 0x01C) >> 2;

    uint32_t address = ((cpu->getRegister(PC_REGISTER) + 2) & 0xFFFFFFFD) + offset;
    uint32_t value = aluShiftRor(cpu->bus->read32(address & 0xFFFFFFFC, 
                                 Bus::CycleType::NONSEQUENTIAL), 
                                 (address & 3) * 8);
                                 
    DEBUG("rd " << (uint32_t)rd << "\n");
    DEBUG("offset " << (uint32_t)offset << "\n");
    cpu->setRegister(rd, value);
    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return NONSEQUENTIAL;
}
