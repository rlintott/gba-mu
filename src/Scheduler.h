#include <cstdint>
#include <list>
#include <array>


class Scheduler {

    public: 
        //Scheduler();

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

            VBLANK_END = 10,
            HBLANK_END = 11,

            NULL_EVENT = 12
        };

        enum EventCondition {
            DMA3_VIDEO_MODE = 0,
            VBLANK_START = 1,
            HBLANK_START = 2,
            NULL_CONDITION = 3
        };

        struct Event {
            EventType eventType;
            uint64_t startCycle;
            bool active = true;
        };

        void addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition EventCondition);
        void removeEvent(EventType eventType);

        /*
            get next event with a cycle start less than currentCycle. Removes the event from the queue

            If eventCondition != NONE, returned event is guaranteed to be one of [DMA0 - DMA3]
        */
        EventType getNextEvent(uint64_t currentCycle, EventCondition eventCondition);

        void printEventList();

    private: 
        struct EventNode {
            Event event;
            EventNode* next = nullptr;
            EventNode* prev = nullptr;
        };

         std::array<EventNode, 12> events = {{
                                    {{HBLANK, 0, false}, nullptr, nullptr}, 
                                    {{VBLANK, 0, false}, nullptr, nullptr},
                                    {{DMA0, 0, false}, nullptr, nullptr},
                                    {{DMA1, 0, false}, nullptr, nullptr},
                                    {{DMA2, 0, false}, nullptr, nullptr},
                                    {{DMA3, 0, false}, nullptr, nullptr},
                                    {{TIMER0, 0, false}, nullptr, nullptr},
                                    {{TIMER1, 0, false}, nullptr, nullptr},
                                    {{TIMER2, 0, false}, nullptr, nullptr},
                                    {{TIMER3, 0, false}, nullptr, nullptr},
                                    {{VBLANK_END, 0, false}, nullptr, nullptr},
                                    {{HBLANK_END, 0, false}, nullptr, nullptr}
                                }};

        EventNode* startNode = nullptr;
        
        //std::list<Event> events;
        std::array<Event, 4> vBlankEvents = {{{DMA0, 0, false},{DMA1, 0, false},{DMA2, 0, false},{DMA3, 0, false}}};
        std::array<Event, 4> hBlankEvents = {{{DMA0, 0, false},{DMA1, 0, false},{DMA2, 0, false},{DMA3, 0, false}}};
        std::array<Event, 1> dma3VideoModeEvents = {{{DMA3, 0, false}}};

        //std::list<Event>::iterator frontIt = events.begin();
        uint64_t currClockCycles;

        EventType getNextConditionalEvent(EventCondition eventCondition);

        bool eventsQueueDirty = false;

        void removeNode(EventNode* eventNode);
        
};