#include <string>
#include "GameBoyAdvance.h"
#include "ARM7TDMI.h"
#include "Bus.h"
#include "PPU.h"
#include "Timer.h"
#include <string.h>



int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cerr << "Please include cpu state as argument" << std::endl;
        exit(1);
    }

    bool isThumb = false;
    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-t") == 0) {
            isThumb = true;
        }
    }

    char* token = strtok(argv[1], "  ");
    std::vector<uint32_t> values;
    while(token != NULL) {
        uint32_t a = std::stoul(std::string(token), nullptr, 16);
        //printf("%08X\n", a); 
        values.push_back(a);
        token = strtok(NULL, " ");
    }

    PPU ppu;
    Bus bus{&ppu};
    ARM7TDMI cpu;
    Timer timer;
    
    GameBoyAdvance gba(&cpu, &bus, &timer);
    
    if(isThumb) {
        cpu.cpsr.T = 1;
    } else {
        cpu.cpsr.T = 0;
    }

    cpu.setCurrInstruction(values[0]);
    cpu.setRegisterDebug(0, values[1]);
    cpu.setRegisterDebug(1, values[2]);
    cpu.setRegisterDebug(2, values[3]);
    cpu.setRegisterDebug(3, values[4]);
    cpu.setRegisterDebug(4, values[5]);
    cpu.setRegisterDebug(5, values[6]);
    cpu.setRegisterDebug(6, values[7]);
    cpu.setRegisterDebug(7, values[8]);
    cpu.setRegisterDebug(8, values[9]);
    cpu.setRegisterDebug(9, values[10]);
    cpu.setRegisterDebug(10, values[11]);
    cpu.setRegisterDebug(11, values[12]);
    cpu.setRegisterDebug(12, values[13]);
    cpu.setRegisterDebug(13, values[14]);
    cpu.setRegisterDebug(14, values[15]);
    cpu.setRegisterDebug(15, values[16]);
    // TODO: cpsr
    //cpu.cpsr.raw = values[17];

    cpu.step();

    printf("~~~~~~~~~~~~~~~~~~ CPU state after instruction: ~~~~~~~~~~~~~~~~~~\n");

    printf("%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
            cpu.getCurrentInstruction(), 
            cpu.getRegisterDebug(0), cpu.getRegisterDebug(1), cpu.getRegisterDebug(2), cpu.getRegisterDebug(3), 
            cpu.getRegisterDebug(4), cpu.getRegisterDebug(5), cpu.getRegisterDebug(6), cpu.getRegisterDebug(7),
            cpu.getRegisterDebug(8), cpu.getRegisterDebug(9), cpu.getRegisterDebug(10), cpu.getRegisterDebug(11),
            cpu.getRegisterDebug(12), cpu.getRegisterDebug(13), cpu.getRegisterDebug(14), cpu.getRegisterDebug(15) + 4, 
            cpu.psrToInt(cpu.getCpsr()));

    return 0;
}

