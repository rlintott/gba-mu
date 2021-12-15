#include "../ARM7TDMI.h"
#include "../../memory/Bus.h"

#include "assert.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armBdtHandler(uint32_t instruction, ARM7TDMI* cpu) {
    // TODO: data aborts (if even applicable to GBA?)
    assert((instruction & 0x0E000000) == 0x08000000);
    // base register
    uint8_t rn = cpu->getRn(instruction);
    // align memory address;
    uint32_t rnVal = cpu->getRegister(rn);
    assert(rn != 15);

    constexpr bool p = op & 0x100;
    constexpr bool u = op & 0x080;
    constexpr bool l = op & 0x010;
    // special case for block transfer, s = what is usually b
    constexpr bool s = op & 0x040;
    bool w = dataTransGetW(instruction);

    if(!(instruction & 0x0000FFFF)) {
        // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5).
        instruction |= 0x00008000;
        // manually setting write bit to false so wont overwrite rn
        w = false;
        if constexpr(u) {
            cpu->setRegister(rn, rnVal + 0x40);
        } else {
            cpu->setRegister(rn, rnVal - 0x40);
        }
    }

    if constexpr (s) assert(cpu->cpsr.Mode != USER);
    uint16_t regList = (uint16_t)instruction;
    uint32_t addressRnStoredAt = 0;  // see below
    bool firstAccess = true;

    if constexpr(u) {
        // up, add offset to base
        if constexpr(p) {
            // pre-increment addressing
            rnVal += 4;
        }
        for (int reg = 0; reg < 16; reg++) {
            if (regList & 1) {
                if constexpr(l) {
                    // LDM{cond}{amod} Rn{!},<Rlist>{^}  ;Load  (Pop)
                    uint32_t data;
                    if(firstAccess) {
                        data = cpu->bus->read32(rnVal & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL);
                        firstAccess = false;
                    } else {
                        data = cpu->bus->read32(rnVal & 0xFFFFFFFC, Bus::CycleType::SEQUENTIAL);
                    }
                    if constexpr(!s) {
                        cpu->setRegister(reg, data);
                    } else {
                        cpu->setUserRegister(reg, data);
                    }
                    if(reg == rn) {
                        // when base is in rlist no writeback (LDM/ARMv4),
                        w = false;
                    }
                } else {
                    // STM{cond}{amod} Rn{!},<Rlist>{^}  ;Store (Push)
                    if (reg == rn) {
                        // hacky!
                        addressRnStoredAt = rnVal;
                    }
                    uint32_t data;
                    if constexpr(!s) {
                        data = cpu->getRegister(reg);
                    } else {
                        data = cpu->getUserRegister(reg);
                    }  
                    // TODO: take this out when implemeinting pipelining
                    if (reg == 15) data += 8;

                    if(firstAccess) {
                        cpu->bus->write32(rnVal & 0xFFFFFFFC, data, Bus::CycleType::NONSEQUENTIAL);
                        firstAccess = false;
                    } else {
                        cpu->bus->write32(rnVal & 0xFFFFFFFC, data, Bus::CycleType::SEQUENTIAL);
                    }
                }
                rnVal += 4;
            }
            regList >>= 1;
        }
        if constexpr(p) {
            // adjust the final rnVal so it is correct
            rnVal -= 4;
        }

    } else {
        // down, subtract offset from base
        if constexpr(p) {
            // pre-increment addressing
            rnVal -= 4;
        }
        for (int reg = 15; reg >= 0; reg--) {
            if (regList & 0x8000) {
                if constexpr(l) {
                    // LDM{cond}{amod} Rn{!},<Rlist>{^}  ;Load  (Pop)
                    uint32_t data;
                    if(firstAccess) {
                        data = cpu->bus->read32(rnVal & 0xFFFFFFFC, Bus::CycleType::NONSEQUENTIAL);
                        firstAccess = false;
                    } else {
                        data = cpu->bus->read32(rnVal & 0xFFFFFFFC, Bus::CycleType::SEQUENTIAL);            
                    }
                    if constexpr(!s) {
                        cpu->setRegister(reg, data);
                    } else {
                        cpu->setUserRegister(reg, data);
                    }                    
                    if(reg == rn) {
                        // when base is in rlist no writeback (LDM/ARMv4),
                        w = false;
                    }
                } else {
                    // STM{cond}{amod} Rn{!},<Rlist>{^}  ;Store (Push)
                    if (reg == rn) {
                        // hacky!
                        addressRnStoredAt = rnVal;
                    }

                    uint32_t data;
                    if constexpr(!s) {
                        data = cpu->getRegister(reg);
                    } else {
                        data = cpu->getUserRegister(reg);
                    } 
                    if (reg == 15) data += 8;
                    if(firstAccess) {
                        cpu->bus->write32(rnVal & 0xFFFFFFFC, data, Bus::CycleType::NONSEQUENTIAL);
                        firstAccess = false;
                    } else {
                        cpu->bus->write32(rnVal & 0xFFFFFFFC, data, Bus::CycleType::SEQUENTIAL);
                    }
                }
                rnVal -= 4;
            }
            regList <<= 1;
        }
        if constexpr(p) {
            // adjust the final rnVal so it is correct
            rnVal += 4;
        }
    }

    if(w) {
        if constexpr(!l) {
            if(((uint16_t)(instruction << (15 - rn)) > 0x8000)) {
                // check if base is not first reg to be stored
                // A STM which includes storing the base, with the base
                // as the first register to be stored, will therefore
                // store the unchanged value, whereas with the base second
                // or later in the transfer order, will store the modified value.
                assert(addressRnStoredAt != 0);
                // TODO: how to tell of sequential or nonsequential
                cpu->bus->write32(addressRnStoredAt & 0xFFFFFFFC, rnVal, Bus::CycleType::SEQUENTIAL);
            }
        }
        cpu->setRegister(rn, rnVal);
    }

    if constexpr(s && l) {
        if((instruction & 0x00008000)) {
            // f instruction is LDM and R15 is in the list: (Mode Changes)
            // While R15 loaded, additionally: CPSR=SPSR_<current mode>
            // TODO make sure to switch mode ANYWHERE where cpsr is set
            cpu->cpsr = *(cpu->getCurrentModeSpsr());
            cpu->switchToMode(ARM7TDMI::Mode(cpu->cpsr.Mode));
        }
    }

    cpu->bus->addCycleToExecutionTimeline(Bus::CycleType::INTERNAL, 0, 0);
    if constexpr(l) {
        if((instruction & 0x00008000)) {
            // LDM with PC in list
            return BRANCH;
        } else {
            return NONSEQUENTIAL;
        }
    } else {
        return NONSEQUENTIAL;
    }



}