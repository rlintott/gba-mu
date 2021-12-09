#include "Scheduler.h"
#include "assert.h"
#include "GameBoyAdvance.h"
#include <iostream>

#define NDEBUG 1;
//#define NDEBUGWARN 1;

#ifdef NDEBUG
#define DEBUG(x)
#else
#define DEBUG(x)        \
    do {                \
        std::cout << "INFO: " << x; \
    } while (0)
#endif


#ifdef NDEBUGWARN
#define DEBUGWARN(x)
#else
#define DEBUGWARN(x)        \
    do {                \
        std::cout << "WARN: " << x; \
    } while (0)
#endif

// Scheduler::Scheduler() {
//     // events.push_back({NULL_EVENT, 0xFFFFFFFFFFFFFFFF, false});
//     // frontIt = events.begin();
// }


void Scheduler::addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition eventCondition) {
    if(eventsQueueDirty && frontIt != events.begin()) {
        //DEBUGWARN("in here!\n");
        events.erase(events.begin(), frontIt);
        frontIt = events.begin();
        eventsQueueDirty = false;
    }

    if(eventCondition == NULL_CONDITION) {
        uint64_t startAt = GameBoyAdvance::cyclesSinceStart + cyclesInFuture;

        std::list<Event>::iterator it = events.begin();
        bool foundPos = false;

        while(it != events.end() && !foundPos) {
            if(startAt < it->startCycle) {
                foundPos = true;
            } else if(startAt == it->startCycle && eventType < it->eventType) {
                foundPos = true;
            } 
            if(!foundPos) {
                ++it;
            }
            
        }
        events.insert(it, {eventType, startAt, true});
        frontIt = events.begin();
    } else {

        switch(eventCondition) {
            case EventCondition::DMA3_VIDEO_MODE: {
                dma3VideoModeEvents[eventType] = {eventType, 0, true};
                break;
            }
            case EventCondition::VBLANK_START: {
                vBlankEvents[eventType] = {eventType, 0, true};
                break;
            }
            case EventCondition::HBLANK_START: {
                hBlankEvents[eventType] = {eventType, 0, true};
                break;
            }
            case EventCondition::NULL_CONDITION: {
                break;
            }
        }
    }
}


Scheduler::EventType Scheduler::getNextEvent(uint64_t currentCycle, EventCondition eventCondition) {
    EventType toReturn = EventType::NULL_EVENT;

    //DEBUGWARN(currentCycle << "\n");
    //DEBUGWARN(events.size() << "\n");
    //printEventList();
    //DEBUGWARN("hi" << "\n");
    if(eventCondition == EventCondition::NULL_CONDITION) {
        
        if(frontIt != events.end()) {
            
            if(frontIt->startCycle <= currentCycle) {
                toReturn = frontIt->eventType;
                ++frontIt;
                eventsQueueDirty = true;
            }
        }
    } else {
        toReturn = getNextConditionalEvent(eventCondition);
    }
    //DEBUGWARN(toReturn << "\n");
    return toReturn;
}

Scheduler::EventType Scheduler::getNextConditionalEvent(EventCondition eventCondition) {
    EventType toReturn = EventType::NULL_EVENT;

    switch(eventCondition) {
        case VBLANK_START: {
            for(int i = 0; i < vBlankEvents.size(); i++) {
                if(vBlankEvents[i].active) {
                    toReturn = vBlankEvents[i].eventType;
                    vBlankEvents[i].active = false;
                    break;
                }
            }
            break;
        }
        case HBLANK_START: {
            for(int i = 0; i < hBlankEvents.size(); i++) {
                if(vBlankEvents[i].active) {
                    DEBUGWARN("heyo\n");
                    toReturn = vBlankEvents[i].eventType;
                    vBlankEvents[i].active = false;
                    break;
                }
            }
            break;
        }
        case DMA3_VIDEO_MODE: {
            for(int i = 0; i < dma3VideoModeEvents.size(); i++) {
                if(vBlankEvents[i].active) {
                    toReturn = vBlankEvents[i].eventType;
                    vBlankEvents[i].active = false;
                    break;
                }
            }
            break;
        }
        default: {
            assert(false);
        }

    }
    return toReturn;
}

void Scheduler::removeEvent(EventType eventType) {
    std::list<Event>::iterator it = frontIt;
    bool found = false;
    while(it != events.end() && !found) {
        if(it->eventType == eventType) {
            found = true;
        } 
        ++it;
    } 
    if(found) {
        events.erase(it);
    }

    if(DMA0 <= eventType && eventType <= DMA3) {
        vBlankEvents[eventType - 2].active = false;
        hBlankEvents[eventType - 2].active = false;
        if(eventType == DMA3) {
            dma3VideoModeEvents[0].active = false;
        }
    }
}

void Scheduler::printEventList() {
    std::cout << "[\n";
    std::list<Event>::iterator it = frontIt;
    while(it != events.end()) {
        std::cout << "{eventType: " << (it->eventType) << " startCycle: " << it->startCycle << " active: " << it->active << "},\n";
        ++it;
    }
    std::cout << "]\n";
    //int i = 5;
}  