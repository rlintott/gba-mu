#include "Bus.h"
#include "GameBoyAdvance.h"
#include "LCD.h"
#include "PPU.h"
#include "DMA.h"
#include "Timer.h"
#include "ARM7TDMI.h"
#include "Scheduler.h"
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

ARM7TDMI cpu;

void handler(int sig) {
    void *array[50];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 50);
    printf("~~~~~~~~~~~~~~~~~~ Uh oh! Error! Printing final instructions ~~~~~~~~~~~~~~~~~~\n");
    // TODO: DO THING WITH DEBUGGER

    printf("%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
            cpu.getCurrentInstruction(), 
            cpu.getRegister(0), cpu.getRegister(1), cpu.getRegister(2), cpu.getRegister(3), 
            cpu.getRegister(4), cpu.getRegister(5), cpu.getRegister(6), cpu.getRegister(7),
            cpu.getRegister(8), cpu.getRegister(9), cpu.getRegister(10), cpu.getRegister(11),
            cpu.getRegister(12), cpu.getRegister(13), cpu.getRegister(14), cpu.getRegister(15) + 4, 
            cpu.psrToInt(cpu.getCpsr()));

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(int argc, char *argv[]) {
    signal(SIGABRT, handler);
    signal(SIGTRAP, handler);
    signal(SIGSEGV, handler);
    bool success = true;
    if(argc < 2) {
        std::cerr << "Please include path to a GBA ROM" << std::endl;
        success = false;
    } else {
        PPU ppu;
        LCD screen;
        DMA dma;
        Timer timer;
        Bus bus{&ppu};
        Scheduler scheduler;

        GameBoyAdvance gba(&cpu, &bus, &screen, &ppu, &dma, &timer, &scheduler);
        if(gba.loadRom(argv[1])) {
            gba.loop();
        }
        else {
            success = false;
        }
    }
    return success;
}

