#pragma once

#include <string>
#include <memory>
#include "Scheduler.h"

class ARM7TDMI;
class Bus;
class LCD;
class PPU;
class DMA;
class Timer;
class Debugger;


class GameBoyAdvanceImpl {
   public:

    GameBoyAdvanceImpl();

    bool loadRom(std::string path);
    void enterMainLoop();
    void printCpuState();

    ARM7TDMI* getCpu();

    static uint64_t cyclesSinceStart;

   private:
    std::shared_ptr<ARM7TDMI> arm7tdmi;
    std::shared_ptr<Bus> bus;
    std::shared_ptr<LCD> screen;
    std::shared_ptr<PPU> ppu;
    std::shared_ptr<DMA> dma;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<Debugger> debugger;
    std::shared_ptr<Scheduler> scheduler;

    uint64_t getTotalCyclesElapsed();
    void testDisplay();

    void dmaXEvent(uint8_t x, Scheduler::Event* dmaEvent, uint16_t currentScanline);

    bool hBlank = false;
    bool scanlineRendered = false;
    bool vBlank = false;

    long previousTime = 0;
    long currentTime = 0;
    long frames = 0;
    double startTimeSeconds = 0.0;
    uint64_t totalCycles= 0;

    bool debugMode = false;

};

