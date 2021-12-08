#include "Scheduler.h"


void Scheduler::addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition eventCondition) {

    if(eventCondition == NONE) {

        uint64_t startAt = currClockCycles + cyclesInFuture;

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
        events.insert(it, {eventType, startAt, eventCondition});
    } else {

        switch(eventCondition) {
            case EventCondition::DMA3_VIDEO_MODE: {
                dma3VideoModeEvents[eventType] = {eventType, 0, eventCondition};
                break;
            }
            case EventCondition::VBLANK: {
                vBlankEvents[eventType] = {eventType, 0, eventCondition};
                break;
            }
            case EventCondition::HBLANK: {
                hBlankEvents[eventType] = {eventType, 0, eventCondition};
                break;
            }
            case EventCondition::NONE: {
                break;
            }
        }
    }
}


Scheduler::EventType Scheduler::getNextEvent(uint64_t currentCycle) {

    std::list<Event>::iterator it = events.begin();
    if(it != events.end()) {
        
        if(it->startCycle <= currentCycle) {
            EventType type = it->eventType;
            events.erase(it);   
            return type;
        }
    }

    return EventType::NONE;
}