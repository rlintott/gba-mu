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
    cpu.setRegister(0, values[1]);
    cpu.setRegister(1, values[2]);
    cpu.setRegister(2, values[3]);
    cpu.setRegister(3, values[4]);
    cpu.setRegister(4, values[5]);
    cpu.setRegister(5, values[6]);
    cpu.setRegister(6, values[7]);
    cpu.setRegister(7, values[8]);
    cpu.setRegister(8, values[9]);
    cpu.setRegister(9, values[10]);
    cpu.setRegister(10, values[11]);
    cpu.setRegister(11, values[12]);
    cpu.setRegister(12, values[13]);
    cpu.setRegister(13, values[14]);
    cpu.setRegister(14, values[15]);
    cpu.setRegister(15, values[16]);
    // TODO: cpsr
    //cpu.cpsr.raw = values[17];

    cpu.step();
    return 0;
}

