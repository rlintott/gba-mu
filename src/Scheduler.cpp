#include "Scheduler.h"
#include "GameBoyAdvance.h"
#include <iostream>
#include "assert.h"

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


void Scheduler::addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition eventCondition, bool ignoreCondition) {
    //DEBUGWARN(events.size() << " after\n");
    // DEBUGWARN(eventType << " :adding event\n");
    // printEventList();
    uint64_t startAt = GameBoyAdvance::cyclesSinceStart + cyclesInFuture;

    EventNode* node = &events[eventType];
    node->event.active = true;
    node->event.startCycle = startAt;
    node->next = nullptr;
    node->event.eventCondition = eventCondition;

    if(startNode == nullptr && eventCondition == NULL_CONDITION) {
        startNode = node;
    } else {
        removeNode(node);
        EventNode* curr = startNode;
        EventNode* prev = nullptr;
        while(curr != nullptr) {
            if(eventCondition == NULL_CONDITION || ignoreCondition) {
                if((curr->event.startCycle > startAt) || 
                (curr->event.startCycle == startAt && eventType < curr->event.eventType)) {
                    break;
                } 
            } else {
                bool found = false;
                switch(eventCondition) {
                    case EventCondition::HBLANK_START:
                    case EventCondition::DMA3_VIDEO_MODE: {
                        if(curr->event.eventType == EventType::HBLANK ) {
                            found = true;
                            node->event.startCycle = curr->event.startCycle;
                        }
                        break;
                    }
                    case EventCondition::VBLANK_START: {
                        if(curr->event.eventType == EventType::VBLANK) {
                            found = true;
                            node->event.startCycle = curr->event.startCycle;
                        }                                
                        break;
                    }
                    default: {
                        assert(false);
                        break;
                    }
                }
                if(found) {
                    // putting the event condition event *after* the event that activates the condition
                    prev = curr;
                    curr = curr->next;
                    while((curr->event.eventType < eventType) && curr->event.eventCondition == eventCondition) {
                        // make sure DMA priority is maintained
                        curr = curr->next;
                    }
                    break;
                }
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
    // printEventList();
    // DEBUGWARN(eventType << " :after adding event\n");

}

void Scheduler::removeNode(EventNode* eventNode) {
    // DEBUGWARN(eventNode->event.eventType << " :removing\n");
    // printEventList();
    if(startNode == nullptr) {
        startNode = nullptr;
    } else if(startNode == eventNode){
        startNode = startNode->next;
    } else {
        EventNode* curr = startNode;
        while(curr->next != nullptr) {
            if(curr->next == eventNode) {
                curr->next = curr->next->next;
                break;
            } 
            curr = curr->next;
        }
    }
    eventNode->next = nullptr;
    // printEventList();
    // DEBUGWARN(eventNode->event.eventType << " :removed\n");
}

Scheduler::Event* Scheduler::getNextEvent(uint64_t currentCycle, EventCondition eventCondition) {
    Event* toReturn = nullptr;
    //if(eventCondition == EventCondition::NULL_CONDITION) {
    if(startNode != nullptr && startNode->event.startCycle <= currentCycle) {
        // DEBUGWARN(startNode->event.eventType << " found get next event\n");
        // printEventList();
        toReturn = &startNode->event;
        EventNode* oldStart = startNode;
        startNode = startNode->next;
        oldStart->next = nullptr;
        // printEventList();
        // DEBUGWARN(toReturn << " after removing event\n");
    }    
    // } else {
    //     if(startNode != nullptr && startNode->event.startCycle <= currentCycle) {
    //         EventNode* curr = startNode;
    //         while(curr != nullptr &&
    //               curr->event.eventCondition != eventCondition && 
    //               curr->event.startCycle <= currentCycle) {
    //             curr = curr->next;
    //         }
    //         // DEBUGWARN(startNode->event.eventType << " found get next event\n");
    //         // printEventList();
    //         if(curr != nullptr && curr->event.startCycle <= currentCycle && curr->event.eventCondition == eventCondition) {
    //             toReturn = curr->event.eventType;
    //             removeNode(curr);
    //         }
    //         // printEventList();
    //         // DEBUGWARN(toReturn << " after removing event\n");
    //     }    

    // }
    //DEBUGWARN(toReturn << "\n");
    return toReturn;
}



Scheduler::Event* Scheduler::peekNextEvent() {
    Event* toReturn = nullptr;
    if(startNode != nullptr) {
        toReturn = &startNode->event;
    }
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
    EventNode* node = &events[eventType];
    removeNode(node);

    // if(DMA0 <= eventType && eventType <= DMA3) {
    //     vBlankEvents[eventType - 2].active = false;
    //     hBlankEvents[eventType - 2].active = false;
    //     if(eventType == DMA3) {
    //         dma3VideoModeEvents[0].active = false;
    //     }
    // }
}

void Scheduler::printEventList() {
    std::cout << "[\n";
    EventNode* curr = startNode;
    int size = 0;
    while(curr != nullptr) {
        size++;
        std::cout << "{eventType: " << (curr->event.eventType) 
                  << " startCycle: " << curr->event.startCycle 
                  << " active: " << curr->event.active 
                  <<  " eventCond: " << curr->event.eventCondition 
                  << "},\n";
        curr = curr->next;
    }
    std::cout << "]\n";

}  