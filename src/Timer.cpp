#include "Timer.h"
#include "Bus.h"

void Timer::step(uint64_t cyclesElapsed) {
    stepTimerX(cyclesElapsed, 0);
    stepTimerX(cyclesElapsed, 1);
    stepTimerX(cyclesElapsed, 2);
    stepTimerX(cyclesElapsed, 3);
}

void Timer::stepTimerX(uint64_t cyclesElapsed, uint8_t x) {
    if(timerStart[x] && !timerCountUp[x]) {
        uint32_t increments = (cyclesElapsed + timerExcessCycles[x]) / timerPrescaler[x];
        timerExcessCycles[x] = cyclesElapsed - (increments * timerPrescaler[x]); // new excess cycles
        timerCounter[x] += increments; 

        if(timerCounter[x] > 0xFFFF) { // if overflow
            // TODO: timer interrupts
            timerCounter[x] = timerReload[x];
            uint8_t cascadeX = x + 1;
            bool overflow = true;
            while(cascadeX <= 3 && timerCountUp[cascadeX] && overflow && timerStart[cascadeX]) {
                timerCounter[cascadeX]++;
                if(timerCounter[cascadeX] > 0xFFFF) {
                    // TODO: timer interrupts
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
    while(width > 0) {
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
                setTimerXReloadLo(byte, 2);
                break;
            }
            case 0x4000109: {
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
    timerStart[x] = val & 0x80;

}

void Timer::setTimerXReloadHi(uint8_t val, uint8_t x) {
    timerReload[x] = (timerReload[x] & 0x00FF) & ((uint16_t)val << 8); 
}

void Timer::setTimerXReloadLo(uint8_t val, uint8_t x) {
    timerReload[x] = (timerReload[x] & 0xFF00) & (uint16_t)val; 
}