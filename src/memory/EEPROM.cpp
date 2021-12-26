#include "EEPROM.h"
#include "../util/macros.h"

#include "assert.h"


// thanks to https://densinh.github.io/DenSinH/emulation/2021/02/01/gba-eeprom.html
// for the explanation of EEPROM 

void EEPROM::transferBitToEeprom(bool bit) {
    if(currTransferBit == 0) {

        readyToRead = false;
        writeComplete = false;
        firstBit = bit;
    } else if(currTransferBit == 1) {

        currAddressBit = busWidth - 1;
        address = 0;
        op = (uint8_t)firstBit | ((uint8_t)bit << 1);
        if(op == READ_OP) {
            // read
            currTransferSize = readSize;
        } else if(op == WRITE_OP) {    
            // write
            currWriteValueBit = 63;
            valueToWrite = 0;
            currTransferSize = writeSize;
        } else {
            DEBUGWARN((uint32_t)op << " :invalid eeprom op\n");
        }
    } else if(currTransferBit < (currTransferSize - 1)) {

        if(op == READ_OP) {
            // read 
            address |= (((uint32_t)bit) << currAddressBit);
            currAddressBit -= 1;
        } else {
            // write
            if(currAddressBit >= 0) {
                address |= (((uint32_t)bit) << currAddressBit);
                currAddressBit -= 1;
            } else if(currWriteValueBit >= 0){
                valueToWrite |= (((uint32_t)bit) << currWriteValueBit);
                currWriteValueBit -= 1;
            }
        }
    } else {

        // transfer over
        currTransferBit = -1;
        if(op == READ_OP) {
            // read
            valueToRead = eeprom[address & 0x3FF];
            readyToRead = true;
        } else {
            // write
            eeprom[address & 0x3FF] = valueToWrite;
            writeComplete = true;
        }
        op = 0;
    }
    currTransferBit++;
}

uint32_t EEPROM::receiveBitFromEeprom() {
    uint32_t returnBit = 0;
    if(writeComplete) {

        // TODO: it'll take ca. 108368 clock cycles (ca. 6.5ms) 
        // until the old data is erased and new data is programmed.
        currReceivingBit = 0;
        returnBit = 1;
    } else if(!readyToRead) {

        currReceivingBit = 0;
        returnBit = 1;
    } else {

        // ready to read
        if(currReceivingBit < 4) {
            // do nothing
            currReceivingBit++;
            returnBit = 0;
        } else if(currReceivingBit < 68) {
            currReceivingBit++;
            returnBit = (valueToRead >> currReadValueBit) & 0x1;;
            currReadValueBit -= 1;
        } else {
            currReadValueBit = 63;
            currReceivingBit = 1;
        }
    }
    return returnBit;
}

void EEPROM::setBusWidth(uint32_t width) {
    assert(width == 6 || width == 14);

    if(width == 6) {
        busWidth = 6;
        writeSize = SIX_BIT_WRITE_SIZE;
        readSize = SIX_BIT_READ_SIZE;
    } else {    
        // wdith == 14
        busWidth = 14;
        writeSize = FOURTEEN_BIT_WRITE_SIZE;
        readSize = FOURTEEN_BIT_READ_SIZE;
    }
}