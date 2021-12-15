#include "../include/GameBoyAdvance.hpp"
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

GameBoyAdvance gba;

void handler(int sig) {
    void *array[50];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 50);
    printf("~~~~~~~~~~~~~~~~~~ Uh oh! Error! Printing final CPU state ~~~~~~~~~~~~~~~~~~\n");

    gba.printCpuState();

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
        if(gba.loadRom(argv[1])) {
            gba.runRom();
        }
        else {
            success = false;
        }
    }
    return success;
}

