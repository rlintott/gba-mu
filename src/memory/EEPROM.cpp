#include "EEPROM.h"
#include "../arm7tdmi/ARM7TDMI.h"

#include "assert.h"


void EEPROM::transferBitToEeprom(bool bit) {
    if(!isReading && !isWriting) {
        DEBUGWARN("eeprom error, neither reading or writing\n");
        assert(false);
    }
    if(ithBit < 2) {
        ithBit += 1;
        return;
    } 
    if(ithBit == transferSize) {
        if(isReading) {
            valueToRead = eeprom[address];
            readyToRead = true;
        } else if(isWriting) {
            eeprom[address] = valueToWrite;
            writeComplete = true;
        }
        // done with the transfer
        isReading = false;
        isWriting = false;
        return;
    }
    if(isReading) {

        address |= ((uint32_t)bit << (currAddressBit));
        currAddressBit -= 1;
        ithBit += 1;
    } else {
        // is writing

        if(currAddressBit >= 0) {
            address |= ((uint32_t)bit << (currAddressBit));
            currAddressBit -= 1;
        } else if(currValueBit >= 0) {
            value |= ((uint32_t)bit << (currValueBit));
            currValueBit -= 1;   
        } else {
        }

        ithBit += 1;
    }
}

uint32_t EEPROM::receiveBitFromEeprom() {
    if(writeComplete) {
        return 1;
    } 

    if(!readyToRead) {
        return 0;
    }

    if(ithBit < 4) {
        ithBit += 1;
        return 0;
    }

    if(ithBit == transferSize) {
        // read over
        return 0;
    }

    uint32_t toReturn = (valueToRead >> (64 + 4 - ithBit - 1)) & 0x1;
    ithBit += 1;
    return toReturn;
}

void EEPROM::startEeprom6BitRead() {
    reset();
    isReading = true;    
    currAddressBit = 5;
    transferSize = SIX_BIT_READ_SIZE;
}

void EEPROM::startEeprom14BitRead() {
    reset();
    isReading = true;    
    currAddressBit = 13;
    transferSize = FOURTEEN_BIT_READ_SIZE;
}

void EEPROM::startEeprom6BitWrite() {
    reset();
    isWriting = true;
    currAddressBit = 5;
    currValueBit = 63;
    transferSize = SIX_BIT_WRITE_SIZE;
}

void EEPROM::startEeprom14BitWrite() {
    reset();
    isWriting = true;
    currAddressBit = 13;
    currValueBit = 63;
    transferSize = FOURTEEN_BIT_WRITE_SIZE;
}

void EEPROM::reset() {
    ithBit = 0;
    isReading = false;
    isWriting = false;
    readyToRead = false;
    writeComplete = false;
    address = 0;
    currAddressBit = 0;
    transferSize = 0;
    currValueBit = 0;
    valueToRead = 0;
    valueToWrite = 0;
}