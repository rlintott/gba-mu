#include "Timer.h"
#include "Bus.h"
#include "ARM7TDMI.h"
#include "Scheduler.h"
#include "GameBoyAdvance.h"



/*
    update IORegister state in bus because about to read from the timer
*/
uint8_t Timer::updateBusToPrepareForTimerRead(uint32_t address, uint8_t width) {
    // manually preparing the memory so that what's read will be up to date
    switch(address) {
        case 0x4000100: {
            calculateTimerXCounter(0, GameBoyAdvance::cyclesSinceStart);
            bus->iORegisters[Bus::IORegister::TM0CNT_L] = timerCounter[0]; 
            bus->iORegisters[Bus::IORegister::TM0CNT_L + 1] = (timerCounter[0]) >> 8; 
            break;
        }
        case 0x4000104: {
            calculateTimerXCounter(1, GameBoyAdvance::cyclesSinceStart);
            bus->iORegisters[Bus::IORegister::TM1CNT_L] = timerCounter[1]; 
            bus->iORegisters[Bus::IORegister::TM1CNT_L + 1] = (timerCounter[1]) >> 8; 
            break;
        }
        case 0x4000108: {
            calculateTimerXCounter(2, GameBoyAdvance::cyclesSinceStart);
            bus->iORegisters[Bus::IORegister::TM2CNT_L] = timerCounter[2]; 
            bus->iORegisters[Bus::IORegister::TM2CNT_L + 1] = (timerCounter[2]) >> 8; 
            break;
        }
        case 0x400010C: {
            calculateTimerXCounter(3, GameBoyAdvance::cyclesSinceStart);
            bus->iORegisters[Bus::IORegister::TM3CNT_L] = timerCounter[3]; 
            bus->iORegisters[Bus::IORegister::TM3CNT_L + 1] = (timerCounter[3]) >> 8; 
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
        //DEBUGWARN(address << " writing to addr\n");
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
                //DEBUGWARN("hi\n");
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
                //DEBUGWARN("hi\n");
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
                //DEBUGWARN("hi\n");
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

inline
void Timer::setTimerXControlHi(uint8_t val, uint8_t x) {
}

inline
void Timer::setTimerXControlLo(uint8_t val, uint8_t x) {
    //((uint32_t) x << "setTimerXControlLo x \n");
    // DEBUGWARN("x: " << (uint32_t)x << "\n");
    // DEBUGWARN("prescaler addr: " << timerPrescaler << "\n");
    Scheduler::EventType timerEvent;
    switch(x) {
        case 0: { timerEvent = Scheduler::EventType::TIMER0; break; }
        case 1: { timerEvent = Scheduler::EventType::TIMER1; break; }
        case 2: { timerEvent = Scheduler::EventType::TIMER2; break; }
        case 3: { timerEvent = Scheduler::EventType::TIMER3; break; }
        default: { break; }
    }

    // remove old event
    scheduler->removeEvent(timerEvent);

    uint8_t prescalerSelection = val & 0x3;
    switch(prescalerSelection) {
        case 0: { timerPrescaler[x] = 1; break; }
        case 1: { timerPrescaler[x] = 64; break; }
        case 2: { timerPrescaler[x] = 256; break; }
        case 3: { timerPrescaler[x] = 1024; break; }
        default: { break; }
    }

    if(!timerStart[x] && (val & 0x80)) {
        // reload value is copied into the counter when the timer start bit becomes changed from 0 to 1.
        timerCounter[x] = timerReload[x];
    }

    uint64_t cyclesPassed = GameBoyAdvance::cyclesSinceStart;

    // update counter
    calculateTimerXCounter(x, cyclesPassed);

    timerCountUp[x] = val & 0x4;
    timerIrqEnable[x] = val & 0x40;
    timerStart[x] = val & 0x80;

    //DEBUGWARN((uint32_t) timerCountUp[x] << " setTimerXControlLo countup \n");
    //DEBUGWARN((uint32_t) timerStart[x] << "setTimerXControlLo start \n");

    if((val & 0x80) && !(val & 0x4)) {
        // only schedule if the timer is not count-up (since count up timers will automatically overflow)
        // timer enabled, so schedule new event
        if(timerCounter[x] > 0xFFFF) { // if overflow
            DEBUGWARN("timer overflowed outside of scheduled event!\n");
            // schedule timer to run immediately
            scheduler->addEvent(timerEvent, 0, Scheduler::EventCondition::NULL_CONDITION);
        } else {
            // add event at time when timer will go off
            //DEBUGWARN("adding timer event to scheduler!\n");
            scheduler->addEvent(timerEvent, 
                                (0x10000 - timerCounter[x]) * timerPrescaler[x], 
                                Scheduler::EventCondition::NULL_CONDITION);
        }
    }
}

inline
void Timer::setTimerXReloadHi(uint8_t val, uint8_t x) {
    timerReload[x] = (timerReload[x] & 0x00FF) | ((uint16_t)val << 8); 
    //DEBUGWARN("timer reload hi in fn " << timerReload[x] << "\n");
}

inline
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

void Timer::connectScheduler(Scheduler* scheduler) {
    this->scheduler = scheduler;
}


void Timer::timerXOverflowEvent(uint8_t x) {
    calculateTimerXCounter(x, GameBoyAdvance::cyclesSinceStart);
    if(timerCounter[x] <= 0xFFFF) {
        DEBUGWARN("timer didn't overflow! scheduling error\n");
        return;
    }

    if(timerIrqEnable[x]) {
        queueTimerInterrupt(x);
    }

    timerCounter[x] = timerReload[x];
    uint8_t cascadeX = x + 1;
    bool overflow = true;
    while(overflow && cascadeX <= 3 && timerCountUp[cascadeX] && timerStart[cascadeX]) {
        timerCounter[cascadeX] += 1;
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
    Scheduler::EventType timerEvent;
    switch(x) {
        case 0: { timerEvent = Scheduler::EventType::TIMER0; break; }
        case 1: { timerEvent = Scheduler::EventType::TIMER1; break; }
        case 2: { timerEvent = Scheduler::EventType::TIMER2; break; }
        case 3: { timerEvent = Scheduler::EventType::TIMER3; break; }
        default: { break; }
    }
    scheduler->addEvent(timerEvent,
                       (0x10000 - timerCounter[x]) * timerPrescaler[x], 
                        Scheduler::EventCondition::NULL_CONDITION);
}

inline
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

inline
void Timer::calculateTimerXCounter(uint8_t x, uint64_t cyclesPassed) {
    // update counter
    if(!timerCountUp[x]) {
        if(timerStart[x]) {
            uint32_t increments = ((cyclesPassed - timerCycleOfLastUpdate[x]) + timerExcessCycles[x]) / timerPrescaler[x];

            if(increments != 0) {
                timerExcessCycles[x] = ((cyclesPassed - timerCycleOfLastUpdate[x]) + timerExcessCycles[x]) % (timerPrescaler[x]);
            } else {
                timerExcessCycles[x] += (cyclesPassed - timerCycleOfLastUpdate[x]);
            }
            
            timerCounter[x] += increments; 
        }
        timerCycleOfLastUpdate[x] = cyclesPassed;
    }
}     