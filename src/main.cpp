#include "Bus.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <bitset>
#include "Debugger.h"


bool loadRawBinary(Bus* bus, std::string path) {
    std::ifstream binFile(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    for(int i = 0; i < buffer.size(); i++) {
        bus->testPushToRam(buffer[i]);
        // DEBUG(std::bitset<8>(buffer[i]).to_string() << " ");
    }
    return true;
}

void printMemory(Bus* bus) {
    for(int i = 0; i < bus->ram.size(); i++) {
        std::cout << (uint32_t)(bus->ram[i]) << '\t';
        if(i % 8 == 7) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}


int main() {
    Bus bus;
    ARM7TDMI cpu;

    loadRawBinary(&bus, "emutest_arm1.bin");

    //printMemory(&bus);

    cpu.connectBus(&bus);

    Debugger debugger(&cpu);

    //std::cout << "Press eneter to step the CPU forward... ";

    debugger.startDebugger();

    while(true) {
        char userInput = '0';
        while(userInput != '\n') {
            std::cin.get(userInput);
        }
        cpu.step();
    }

    printMemory(&bus);

    std::cout << "completed!" << "\n";
    return 0;
}
