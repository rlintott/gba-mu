#include <cstdint>
#include <list>
#include <array>


class Scheduler {

    public: 
        enum EventType {
            DMA0 = 0,
            DMA1 = 1,
            DMA2 = 2,
            DMA3 = 3,
            
            TIMER0 = 4,
            TIMER1 = 5,
            TIMER2 = 6,
            TIMER3 = 7,

            HBLANK = 8,
            VBLANK = 9,

            NONE = 10
        };

        enum EventCondition {
            DMA3_VIDEO_MODE = 0,
            VBLANK = 1,
            HBLANK = 2,
            NONE = 3
        };

        struct Event {
            EventType eventType;
            uint64_t startCycle;
            EventCondition eventCondition;
        };

        void addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition EventCondition);
        void removeEvent(EventType eventType);

        EventType getNextEvent(uint64_t currentCycle);

    private: 
        std::list<Event> events;
        std::array<Event, 4> vBlankEvents;
        std::array<Event, 4> hBlankEvents;
        std::array<Event, 1> dma3VideoModeEvents;

        uint64_t currClockCycles = 0;
};