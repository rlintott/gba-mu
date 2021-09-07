#include <cstdint>


class Bus;


class Timer {

    public: 

        void step(uint64_t cycles);
        uint8_t readFromTimer(uint32_t address);
        void writeToTimer(uint32_t address, uint8_t value);

    private:
        Bus* bus;

        uint64_t timer0Start;
        uint64_t timer1Start;
        uint64_t timer2Start;
        uint64_t timer3Start;

        uint64_t timer0End;
        uint64_t timer1End;
        uint64_t timer2End;
        uint64_t timer3End;

        // these counters are not guaranteed to be accurate 
        // updated on an as needed basis
        uint32_t timer0Counter;
        uint32_t timer1Counter;
        uint32_t timer2Counter;
        uint32_t timer3Counter;

        uint32_t timer0Reload;
        uint32_t timer1Reload;
        uint32_t timer2Reload;
        uint32_t timer3Reload;


};