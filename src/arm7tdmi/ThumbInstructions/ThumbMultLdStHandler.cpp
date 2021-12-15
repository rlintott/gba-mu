#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::thumbMultLdStHandler(uint16_t instruction, ARM7TDMI* cpu) {
    assert((instruction & 0xF000) == 0xC000);
    DEBUG("in THUMB.15: multiple load/store\n");
    // uint8_t opcode = (instruction & 0x0800) >> 11;
    constexpr bool opcode = (op & 0x020);
    uint8_t rList = instruction & 0x00FF;
    //uint8_t rb = (instruction & 0x0700) >> 8;
    constexpr uint8_t rb = (op & 0x01C) >> 2;
    uint32_t rbValue = cpu->getRegister(rb);
    uint32_t oldRbValue = rbValue;
    DEBUG("rbValue: " << rbValue << "\n");

    bool firstAccess = true;

    // In THUMB mode stack is always meant to be 'full descending', 
    // ie. PUSH is equivalent to 'STMFD/STMDB' and POP to 'LDMFD/LDMIA' in ARM mode.

    if constexpr(!opcode) {
        // 0: STMIA Rb!,{Rlist}   ;store in memory, increments Rb (post-increment store)
        for(int i = 0; i < 8; i++) {
            if(rList & 0x01) {
                DEBUG("writing rlist val to mem\n");
                if(firstAccess) {
                    cpu->bus->write32(rbValue, cpu->getRegister(i), Bus::CycleType::NONSEQUENTIAL);
                    firstAccess = false;
                } else {
                    cpu->bus->write32(rbValue, cpu->getRegister(i), Bus::CycleType::SEQUENTIAL);
                }
                rbValue += 4;
            }
            rList >>= 1;
        }
        if(!(instruction & 0x00FF)) {
            // empty rList
            // R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5). 
            cpu->bus->write32(rbValue, cpu->getRegister(PC_REGISTER) + 4, Bus::CycleType::NONSEQUENTIAL);
            rbValue += 0x40;
        }
        if(((uint32_t)rList >> rb) & 0x1) {
            // if rb included in rList, Store OLD base if Rb is FIRST entry in Rlist, 
            // otherwise store NEW base (STM/ARMv4)
            DEBUG("rb included in rlist!\n");
            if(!(rList << (8 - rb))) {
                // rb is first entry in rList
                cpu->setRegister(rb, oldRbValue);   
            } else {
                cpu->setRegister(rb, rbValue);
            }
        } else {
            DEBUG("rb not included in rlist!\n");
            cpu->setRegister(rb, rbValue);
        }    
    } else {
        // 1: LDMIA Rb!,{Rlist}   ;load from memory, increments Rb (post-increment load)
        for(int i = 0; i < 8; i++) {
            if(rList & 0x01) {
                if(firstAccess) {
                    cpu->setRegister(i, cpu->bus->read32(rbValue, Bus::CycleType::NONSEQUENTIAL));
                    firstAccess = false;
                } else {
                    cpu->setRegister(i, cpu->bus->read32(rbValue, Bus::CycleType::SEQUENTIAL));
                }
                rbValue += 4;
            }
            rList >>= 1;
        }
        if(!(instruction & 0x00FF)) {
            // empty rList
            // R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5).
            cpu->setRegister(PC_REGISTER, cpu->bus->read32(rbValue, Bus::CycleType::NONSEQUENTIAL));
            rbValue += 0x40;
        }
        if(((uint32_t)rList >> rb) & 0x1) {
            // if rb included in rList, no write back for stm
        } else {
            cpu->setRegister(rb, rbValue);
        }        
    }

    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    return NONSEQUENTIAL;
}
