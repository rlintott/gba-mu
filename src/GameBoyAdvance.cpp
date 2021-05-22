#include "GameBoyAdvance.h"
#include "ARM7TDMI.h"
#include "Bus.h"

#include <iostream>
#include <fstream>
#include <iterator>

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    arm7tdmi->connectBus(bus);
}

GameBoyAdvance::~GameBoyAdvance() {
}


void GameBoyAdvance::loadRom(std::string path) {
    std::ifstream binFile(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(binFile), {});

    for(int i = 0; i < buffer.size(); i++) {
        bus->tempPushToRam(buffer[i]);
        // DEBUG(std::bitset<8>(buffer[i]).to_string() << " ");
    }
}






