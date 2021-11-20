#include "Timer.h"
#include "Bus.h"
#include "ARM7TDMI.h"

void Timer::step(uint64_t cyclesElapsed) {
    stepTimerX(cyclesElapsed, 0);
    stepTimerX(cyclesElapsed, 1);
    stepTimerX(cyclesElapsed, 2);
    stepTimerX(cyclesElapsed, 3);
}

void Timer::stepTimerX(uint64_t cyclesElapsed, uint8_t x) {
    if(timerStart[x] && !timerCountUp[x]) {
        //DEBUGWARN("cycles elapsed " << cyclesElapsed << "\n");
        //DEBUGWARN("timer prescaper " << timerPrescaler[x] << "\n");
        //DEBUGWARN("timer excess " << timerExcessCycles[x] << "\n");
        uint32_t increments = (cyclesElapsed + timerExcessCycles[x]) / timerPrescaler[x];

        //DEBUGWARN("increments " << increments << "\n");
        if(increments != 0) {
            timerExcessCycles[x] = (cyclesElapsed + timerExcessCycles[x]) % timerPrescaler[x];
        } else {
            timerExcessCycles[x] += cyclesElapsed;
        }
        timerCounter[x] += increments; 

        if(timerCounter[x] > 0xFFFF) { // if overflow
            if(timerIrqEnable[x]) {
                queueTimerInterrupt(x);
            }
            //DEBUGWARN("howdy!\n");
            timerCounter[x] = timerReload[x];
            uint8_t cascadeX = x + 1;
            bool overflow = true;
            while(overflow && cascadeX <= 3 && timerCountUp[cascadeX] && timerStart[cascadeX]) {
                timerCounter[cascadeX]++;
                if(timerCounter[cascadeX] > 0xFFFF) {
                    if(timerIrqEnable[cascadeX]) {
                        queueTimerInterrupt(cascadeX);
                    }
                    timerCounter[cascadeX] = timerReload[cascadeX];
                } else {
                    overflow = false;
                }
                cascadeX++;
            }
        }
    }
}

/*
    update IORegister state in bus because about to read from the timer
*/
uint8_t Timer::updateBusToPrepareForTimerRead(uint32_t address, uint8_t width) {
    // manually preparing the memory so that what's read will be up to date
    switch(address) {
        case 0x4000100: {
            bus->iORegisters[Bus::IORegister::TM0CNT_L] = timerCounter[0]; 
            bus->iORegisters[Bus::IORegister::TM0CNT_L + 1] = timerCounter[0] >> 8; 
            break;
        }
        case 0x4000104: {
            bus->iORegisters[Bus::IORegister::TM1CNT_L] = timerCounter[1]; 
            bus->iORegisters[Bus::IORegister::TM1CNT_L + 1] = timerCounter[1] >> 8; 
            break;
        }
        case 0x4000108: {
            bus->iORegisters[Bus::IORegister::TM2CNT_L] = timerCounter[2]; 
            bus->iORegisters[Bus::IORegister::TM2CNT_L + 1] = timerCounter[2] >> 8; 
            break;
        }
        case 0x400010C: {
            bus->iORegisters[Bus::IORegister::TM3CNT_L] = timerCounter[3]; 
            bus->iORegisters[Bus::IORegister::TM3CNT_L + 1] = timerCounter[3] >> 8; 
            break;
        } 
        default: {
            break;
        }
    }
}

/*
    update Timer state upon write
*/
void Timer::updateTimerUponWrite(uint32_t address, uint32_t value, uint8_t width) {
    while(width != 0) {
        uint8_t byte = value & 0xFF;

        switch(address) {
            case 0x4000100: {
                setTimerXReloadLo(byte, 0);
                break;
            }
            case 0x4000101: {
                setTimerXReloadHi(byte, 0);
                break;
            }
            case 0x4000102: {
                setTimerXControlLo(byte, 0);
                break;
            }
            case 0x4000103: {
                setTimerXControlHi(byte, 0);
                break;
            }
            case 0x4000104: {
                setTimerXReloadLo(byte, 1);
                break;
            }
            case 0x4000105: {
                setTimerXReloadHi(byte, 1);
                break;
            }
            case 0x4000106: {
                setTimerXControlLo(byte, 1);
                break;
            }
            case 0x4000107: {
                setTimerXControlHi(byte, 1);
                break;
            }            
            case 0x4000108: {
                //DEBUGWARN("LO!" << (uint32_t)byte << "\n");
                setTimerXReloadLo(byte, 2);
                break;
            }
            case 0x4000109: {
                //DEBUGWARN("HI!" << (uint32_t)byte << "\n");
                setTimerXReloadHi(byte, 2);
                break;
            }
            case 0x400010A: {
                setTimerXControlLo(byte, 2);
                break;
            }
            case 0x400010B: {
                setTimerXControlHi(byte, 2);
                break;
            }
            case 0x400010C: {
                setTimerXReloadLo(byte, 3);
                break;
            }
            case 0x400010D: {
                setTimerXReloadHi(byte, 3);
                break;
            }
            case 0x400010E: {
                setTimerXControlLo(byte, 3);
                break;
            }
            case 0x400010F: {
                setTimerXControlHi(byte, 3);
                break;
            }
            default: {
                break;
            }
        }

        width -= 8;
        address += 1;
        value = value >> 8;
    }
    return;
}


void Timer::setTimerXControlHi(uint8_t val, uint8_t x) {
}

void Timer::setTimerXControlLo(uint8_t val, uint8_t x) {
    uint8_t prescalerSelection = val & 0x3;
    switch(prescalerSelection) {
        case 0: { timerPrescaler[x] = 1; break; }
        case 1: { timerPrescaler[x] = 64; break; }
        case 2: { timerPrescaler[x] = 256; break; }
        case 3: { timerPrescaler[x] = 1024; break; }
        default: { break; }
    }
    timerCountUp[x] = val & 0x4;
    timerIrqEnable[x] = val & 0x40;
    if(!timerStart[x] && (val & 0x80)) {
        // reload value is copied into the counter when the timer start bit becomes changed from 0 to 1.
        timerCounter[x] = timerReload[x];
    }
    //DEBUGWARN("setting timer start " << (uint32_t)x << " " << (bool)(val & 0x80) << "\n");
    timerStart[x] = val & 0x80;

}

void Timer::setTimerXReloadHi(uint8_t val, uint8_t x) {
    timerReload[x] = (timerReload[x] & 0x00FF) | ((uint16_t)val << 8); 
    //DEBUGWARN("timer reload hi in fn " << timerReload[x] << "\n");
}

void Timer::setTimerXReloadLo(uint8_t val, uint8_t x) {
    timerReload[x] = (timerReload[x] & 0xFF00) | (uint16_t)val; 
}

void Timer::connectBus(Bus* bus) {
    this->bus = bus;
    this->bus->connectTimer(this);
}

void Timer::connectCpu(ARM7TDMI* cpu) {
    this->cpu = cpu;
}

void Timer::queueTimerInterrupt(uint8_t x) {
    switch(x) {
        case 0: {
            cpu->queueInterrupt(ARM7TDMI::Interrupt::Timer0Overflow);
            break;
        }
        case 1: {
            cpu->queueInterrupt(ARM7TDMI::Interrupt::Timer1Overflow);
            break;
        }
        case 2: {
            cpu->queueInterrupt(ARM7TDMI::Interrupt::Timer2Overflow);
            break;
        }
        case 3: {
            cpu->queueInterrupt(ARM7TDMI::Interrupt::Timer3Overflow);
            break;
        }
        default: {
            break;
        }
    }
}