#include <string>

class ARM7TDMI;
class Bus;
class LCD;
class PPU;
class DMA;
class Timer;
class Debugger;
class Scheduler;

class GameBoyAdvance {
   public:

    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen, PPU* _ppu, DMA* _dma, Timer* _timer, Scheduler* _scheduler);
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus);
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, Timer* _timer);
    ~GameBoyAdvance();

    bool loadRom(std::string path);
    void startRom();
    void loop();

    static uint64_t cyclesSinceStart;

   private:
    ARM7TDMI* arm7tdmi;
    Bus* bus;
    LCD* screen;
    PPU* ppu;
    DMA* dma;
    Timer* timer;
    Debugger* debugger;
    Scheduler* scheduler;

    uint64_t getTotalCyclesElapsed();
    void testDisplay();

    bool hBlank = false;
    bool scanlineRendered = false;
    bool vBlank = false;

    long previousTime = 0;
    long currentTime = 0;
    long frames = 0;
    double startTimeSeconds = 0.0;
    uint64_t totalCycles= 0;

    bool debugMode = true;

};