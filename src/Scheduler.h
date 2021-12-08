#include <cstdint>
#include <list>
#include <array>


class Scheduler {

    public: 
        enum EventType {
            HBLANK = 0,
            VBLANK = 1,

            DMA0 = 2,
            DMA1 = 3,
            DMA2 = 4,
            DMA3 = 5,
            
            TIMER0 = 6,
            TIMER1 = 7,
            TIMER2 = 8,
            TIMER3 = 9,

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

        EventType getNextTemporalEvent(uint64_t currentCycle);
        EventType getNextConditionEvent(EventCondition eventCondition);

    private: 
        std::list<Event> events;
        std::array<Event, 4> vBlankEvents;
        std::array<Event, 4> hBlankEvents;
        std::array<Event, 1> dma3VideoModeEvents;

        uint64_t currClockCycles = 0;
};