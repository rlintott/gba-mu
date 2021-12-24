#include "EEPROM.h"
#include "../arm7tdmi/ARM7TDMI.h"

#include "assert.h"


// thanks to https://densinh.github.io/DenSinH/emulation/2021/02/01/gba-eeprom.html
// for the explanation of EEPROM 

void EEPROM::transferBitToEeprom(bool bit) {
    if(totalTransfers == 0) {

        readyToRead = false;
        writeComplete = false;
        firstBit = bit;
    } else if(totalTransfers == 1) {

        currAddressBit = busWidth - 1;
        address = 0;
        op = (uint8_t)firstBit | ((uint8_t)bit << 1);
        if(op == READ_OP) {
            // read
            currTransferSize = FOURTEEN_BIT_READ_SIZE;
        } else if(op == WRITE_OP) {    
            // write
            currWriteValueBit = 63;
            valueToWrite = 0;
            currTransferSize = FOURTEEN_BIT_WRITE_SIZE;
        } else {
            DEBUGWARN((uint32_t)op << " :invalid eeprom op\n");
        }
    } else if(totalTransfers < (currTransferSize - 1)) {

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
        totalTransfers = -1;
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
    totalTransfers++;
}

uint32_t EEPROM::receiveBitFromEeprom() {
    uint32_t returnBit = 0;
    if(writeComplete) {

        // TODO: it'll take ca. 108368 clock cycles (ca. 6.5ms) 
        // until the old data is erased and new data is programmed.
        totalReceives = 0;
        returnBit = 1;
    } else if(!readyToRead) {

        totalReceives = 0;
        returnBit = 1;
    } else {

        // ready to read
        if(totalReceives < 4) {
            // do nothing
            totalReceives++;
            returnBit = 0;
        } else if(totalReceives < 68) {
            totalReceives++;
            returnBit = (valueToRead >> currReadValueBit) & 0x1;;
            currReadValueBit -= 1;
        } else {
            currReadValueBit = 63;
            totalReceives = 1;
        }
    }
    return returnBit;
}

