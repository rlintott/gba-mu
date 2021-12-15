
#include "../include/GameBoyAdvance.hpp"
#include "GameBoyAdvanceImpl.h"
#include <iostream>


GameBoyAdvance::GameBoyAdvance() {
    pimpl = std::make_shared<GameBoyAdvanceImpl>();
}


bool GameBoyAdvance::loadRom(std::string path) {
    return pimpl->loadRom(path);
}

void GameBoyAdvance::enableDebugger() {
    // TODO
} 

void GameBoyAdvance::setBreakpoint(uint32_t address) {
    // TODO
}

void GameBoyAdvance::printCpuState() {
    pimpl->printCpuState();
} 

void GameBoyAdvance::runRom() {
    pimpl->enterMainLoop();
}



