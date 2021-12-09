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
    //DEBUGWARN(events.size() << " after\n");
    if(eventCondition == NULL_CONDITION) {
        uint64_t startAt = GameBoyAdvance::cyclesSinceStart + cyclesInFuture;

        EventNode* node = &events[eventType];
        node->event.active = true;
        node->event.startCycle = startAt;
        node->next = nullptr;

        if(startNode == nullptr) {
            startNode = node;
        } else {
            removeNode(node);

            EventNode* curr = startNode;
            EventNode* prev = nullptr;
            while(curr != nullptr) {
                if((curr->event.startCycle > startAt) || 
                   (curr->event.startCycle == startAt && eventType < curr->event.eventType)) {
                    break;
                } 
                prev = curr;
                curr = curr->next;
            }
            node->next = curr;
            if(prev != nullptr) {
                prev->next = node;
            } else {
                startNode = node;
            }
        }

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

void Scheduler::removeNode(EventNode* eventNode) {
    if(startNode == eventNode) {
        startNode = nullptr;
    } else {
        EventNode* curr = startNode;
        while(curr->next != nullptr) {
            //DEBUGWARN("hey!\n");
            if(curr->next == eventNode) {
                curr->next = curr->next->next;
                break;
            } 
            curr = curr->next;
        }
    }
    eventNode->next = nullptr;
}

Scheduler::EventType Scheduler::getNextEvent(uint64_t currentCycle, EventCondition eventCondition) {
    EventType toReturn = EventType::NULL_EVENT;
    if(eventCondition == EventCondition::NULL_CONDITION) {
        if(startNode != nullptr && startNode->event.startCycle < currentCycle) {
            toReturn = startNode->event.eventType;
            EventNode* oldStart = startNode;
            startNode = startNode->next;
            oldStart->next = nullptr;
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
    //DEBUGWARN("getting called!\n");
    EventNode* node = &events[eventType];
    removeNode(node);

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
    EventNode* curr = startNode;
    while(curr != nullptr) {
        std::cout << "{eventType: " << (curr->event.eventType) << " startCycle: " << curr->event.startCycle << " active: " << curr->event.active << "},\n";
        curr = curr->next;
    }
    std::cout << "]\n";

}  