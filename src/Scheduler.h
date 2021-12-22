
#pragma once

#include <cstdint>
#include <list>
#include <array>

class Scheduler {

    public: 
        //Scheduler();

        enum EventType {
            HBLANK = 0,
            VBLANK = 1,
            
            TIMER0 = 2,
            TIMER1 = 3,
            TIMER2 = 4,
            TIMER3 = 5,

            VBLANK_END = 6,
            HBLANK_END = 7,

            NULL_EVENT = 8,

            DMA0 = 9,
            DMA1 = 10,
            DMA2 = 11,
            DMA3 = 12,
        };

        enum EventCondition {
            DMA3_VIDEO_MODE = 0,
            VBLANK_START = 1,
            HBLANK_START = 2,
            NULL_CONDITION = 3
        };

        static constexpr inline uint8_t convertDmaTypeToDmaVal(EventType dma) {
            return dma - 9;
        };

        static constexpr inline uint8_t convertDmaValToDmaEvent(uint8_t x) {
            return x + 9;
        };


        struct Event {
            EventType eventType;
            uint64_t startCycle;
            bool active = true;
            EventCondition eventCondition;
        };

        void addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition EventCondition, bool ignoreCondition);
        void removeEvent(EventType eventType);

        /*
            get next event with a cycle start less than currentCycle. Removes the event from the queue

            If eventCondition != NONE, returned event is guaranteed to be one of [DMA0 - DMA3]
        */
        Event* getNextEvent(uint64_t currentCycle, EventCondition eventCondition);

        Event* peekNextEvent();

        void printEventList();

    private: 
        struct EventNode {
            Event event;
            EventNode* next = nullptr;
            EventNode* prev = nullptr;
        };

         std::array<EventNode, 13> events = {{
                                    {{HBLANK, 0, false, NULL_CONDITION}, nullptr, nullptr}, 
                                    {{VBLANK, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{TIMER0, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{TIMER1, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{TIMER2, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{TIMER3, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{VBLANK_END, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{HBLANK_END, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{NULL_EVENT, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{DMA0, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{DMA1, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{DMA2, 0, false, NULL_CONDITION}, nullptr, nullptr},
                                    {{DMA3, 0, false, NULL_CONDITION}, nullptr, nullptr}
                                }};

        EventNode* startNode = nullptr;
        
        void removeNode(EventNode* eventNode);
        
};