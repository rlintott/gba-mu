#include "Bus.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <bitset>


bool loadRawBinary(Bus* bus, std::string path) {
    std::ifstream binFile(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    for(int i = 0; i < buffer.size(); i++) {
        bus->testPushToRam(buffer[i]);
        // DEBUG(std::bitset<8>(buffer[i]).to_string() << " ");
    }

    
}


int main() {
    Bus bus;
    ARM7TDMI cpu;

    loadRawBinary(&bus, "emutest_arm1.bin");
    cpu.connectBus(&bus);
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();
    cpu.step();


    std::cout << "completed!" << "\n";
    return 0;
}
