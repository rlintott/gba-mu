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
    bus->loadRom(path);
}






