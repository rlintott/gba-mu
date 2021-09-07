#include "Timer.h"
#include "Bus.h"



void Timer::step(uint64_t cycle) {
    if(cycle >= timer0End) {
        if(false) { // if timer 1 countup
            // increment counter1
        }
    }
    if(cycle >= timer1End) {
        if(false) { // if timer 2 countup
            // increment counter2
        }
    }
    if(cycle >= timer2End) {
        if(false) { // if timer 3 countup
            // increment counter3
        }        
    }
    if(cycle >= timer3End) {
       
    }
}

uint8_t Timer::readFromTimer(uint32_t address) {


}

void Timer::writeToTimer(uint32_t address, uint8_t value) {
    

}