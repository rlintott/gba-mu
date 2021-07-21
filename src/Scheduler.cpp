#include "Scheduler.h"
#include "ARM7TDMI.h"
#include "Bus.h"



void Scheduler::runLoop() {


    while(true) {
        uint32_t cycles = cpu->step();
    }

}

