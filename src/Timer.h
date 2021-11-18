#include <cstdint>


class Bus;
class ARM7TDMI;


class Timer {

    public: 
        void step(uint64_t cyclesElapsed);
        uint8_t updateBusToPrepareForTimerRead(uint32_t address, uint8_t width);
        void updateTimerUponWrite(uint32_t address, uint32_t value, uint8_t width);
        void connectBus(Bus* bus);
        void connectCpu(ARM7TDMI* cpu);

    private:
        void stepTimerX(uint64_t cycles, uint8_t x);

        void setTimerXReloadLo(uint8_t val, uint8_t x);
        void setTimerXReloadHi(uint8_t val, uint8_t x);

        void setTimerXControlLo(uint8_t val, uint8_t x);
        void setTimerXControlHi(uint8_t val, uint8_t x);

        void queueTimerInterrupt(uint8_t x);

        Bus* bus;

        ARM7TDMI* cpu;

        bool timerStart[4] = {false, false, false, false};

        uint32_t timerExcessCycles[4] = {0, 0, 0, 0};

        uint32_t timerCounter[4] = {0, 0, 0, 0};

        uint32_t timerReload[4] = {0, 0, 0, 0};

        uint32_t timerPrescaler[4] = {1, 1, 1, 1};

        bool timerCountUp[4] = {false, false, false, false};

        bool timerIrqEnable[4] = {false, false, false, false};
};