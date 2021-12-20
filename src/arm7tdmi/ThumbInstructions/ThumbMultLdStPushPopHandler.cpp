#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbMultLdStPushPopHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0xB000);
    assert((instruction & 0x0600) == 0x0400);
    //uint8_t opcode = (instruction & 0x0800) >> 11;
    constexpr bool opcode = (op & 0x020);
    //bool pcLrBit = instruction & 0x0100; // 1: PUSH LR (R14), or POP PC (R15)
    constexpr bool pcLrBit = (op & 0x004);
    uint8_t rList = instruction & 0x00FF;
    uint32_t spValue = cpu->getRegister(SP_REGISTER);
    bool firstAccess = true;
    bool branch = false;

    // In THUMB mode stack is always meant to be 'full descending', 
    // ie. PUSH is equivalent to 'STMFD/STMDB' and POP to 'LDMFD/LDMIA' in ARM mode.
    if constexpr(!opcode) {
        // 0: PUSH {Rlist}{LR}   ;store in memory, decrements SP (R13)
        if constexpr(pcLrBit) {
            spValue -=4;
            // TODO: just realized! for push, or backwards block memory, 
            // the first data access in the for loop might be the last memory access in reality
            cpu->bus->write32(spValue, cpu->getRegister(LINK_REGISTER), Bus::CycleType::NONSEQUENTIAL);
            firstAccess = false;
        }
        for(int i = 7; i >= 0; i--) {
            if(rList & 0x80) {
                spValue -= 4;
                if(firstAccess) {
                    cpu->bus->write32(spValue, cpu->getRegister(i), Bus::CycleType::NONSEQUENTIAL);
                    firstAccess = false;
                } else {
                    cpu->bus->write32(spValue, cpu->getRegister(i), Bus::CycleType::SEQUENTIAL);
                }
            }
            rList <<= 1;
        }
        cpu->setRegister(SP_REGISTER, spValue);
    } else {
        // 1: POP  {Rlist}{PC}   ;load from memory, increments SP (R13)
        for(int i = 0; i < 8; i++) {
            if(rList & 0x01) {
                if(firstAccess) {
                    cpu->setRegister(i, cpu->bus->read32(spValue, Bus::CycleType::NONSEQUENTIAL));
                    firstAccess = false;
                } else {
                    cpu->setRegister(i, cpu->bus->read32(spValue, Bus::CycleType::SEQUENTIAL));
                }
                spValue += 4;
            }
            rList >>= 1;
        }  

        if constexpr(pcLrBit) {
            // TODO, whether it's sequentiual or not might depend on whether rlist is empty
            cpu->setRegister(PC_REGISTER, cpu->bus->read32(spValue, Bus::CycleType::SEQUENTIAL));
            spValue += 4;
        } 
        cpu->setRegister(SP_REGISTER, spValue);
    }

    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);

    if(!branch) {
        return BRANCH;
    } else {
        return NONSEQUENTIAL;
    }
}
