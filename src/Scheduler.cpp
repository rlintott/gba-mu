#include "Scheduler.h"
#include "GameBoyAdvanceImpl.h"
#include <iostream>
#include "util/macros.h"

#include "assert.h"


void Scheduler::addEvent(EventType eventType, uint64_t cyclesInFuture, EventCondition eventCondition, bool ignoreCondition) {
    // printEventList();
    uint64_t startAt = GameBoyAdvanceImpl::cyclesSinceStart + cyclesInFuture;

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
                    while(curr != nullptr && 
                          curr->event.eventType < eventType && 
                          curr->event.eventCondition == eventCondition) {
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
}

void Scheduler::removeNode(EventNode* eventNode) {
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
}

Scheduler::Event* Scheduler::getNextEvent(uint64_t currentCycle, EventCondition eventCondition) {
    Event* toReturn = nullptr;

    if(startNode != nullptr && startNode->event.startCycle <= currentCycle) {
        toReturn = &startNode->event;
        EventNode* oldStart = startNode;
        startNode = startNode->next;
        oldStart->next = nullptr;
    }    

    return toReturn;
}

Scheduler::Event* Scheduler::peekNextEvent() {
    Event* toReturn = nullptr;
    if(startNode != nullptr) {
        toReturn = &startNode->event;
    }
    return toReturn;
}

void Scheduler::removeEvent(EventType eventType) {
    EventNode* node = &events[eventType];
    removeNode(node);

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