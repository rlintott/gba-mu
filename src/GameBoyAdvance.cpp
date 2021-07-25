#include "GameBoyAdvance.h"

#include <fstream>
#include <iostream>
#include <iterator>

#include "ARM7TDMI.h"
#include "Bus.h"

GameBoyAdvance::GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus) {
    this->arm7tdmi = _arm7tdmi;
    this->bus = _bus;
    arm7tdmi->connectBus(bus);
}

GameBoyAdvance::~GameBoyAdvance() {}

void GameBoyAdvance::loadRom(std::string path) { 
    bus->loadRom(path); 
    arm7tdmi->initializeWithRom();
}
