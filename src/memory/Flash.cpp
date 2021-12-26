#include "Flash.h"
#include "../util/macros.h"

#include <algorithm> 

#include "assert.h"

void Flash::write(uint32_t address, uint8_t value) {
    if(currMode == WRITE) {

        if(!(address == 0x0E005555 && value == 0xF0)) {
            flash[(address & 0x0000FFFF) + bank] = value;
        }
        currMode = READY;
    } else if(currMode == MEM_BANK) {

        if(address == 0x0E000000 && (value == 0 || value == 1)) {
            bank = value * 0x10000;
        } else {
            DEBUGWARN("currMode error in mem bank command\n");
        }
        currMode = READY;
    } else if(address == 0xE005555 && value == 0xAA) {

        currStage = 0;
    } else if(address == 0xE002AAA && value == 0x55 && currStage == 0) {

        currStage = 1;
    } else if((address == 0xE005555 || is4KbEraseCommand(address, value)) && currStage == 1) {

        // command is being sent
        switch(value) {
            case 0x90: {
                // Enter “Chip identification mode”
                currMode = CHIP_ID;
                temp0x0 = flash[0x0];
                temp0x1 = flash[0x1];
                flash[0x0] = manufacturerId;
                flash[0x1] = deviceId;
                break;
            }
            case 0xF0: {
                // Exit “Chip identification mode”
                currMode = READY;
                flash[0x0] = temp0x0;
                flash[0x1] = temp0x1;
                break;
            }
            case 0x80: {
                // Prepare to receive erase command
                currMode = ERASE;
                break;
            }
            case 0x10: {
                // Erase entire chip
                if(currMode == ERASE) {
                    // TODO: cycle accuracy? (this normally takes some time)
                    flash.fill(0xFF);
                } else {
                    DEBUGWARN("currMode not erase, something wrong in flash!\n");
                }
                break;
            }
            case 0x30: {
                // Erase 4 kilobyte sector
                if(currMode == ERASE) {
                    uint32_t page = (address & 0x0000F000);
                    std::fill_n(flash.begin() + page + bank, 0xFFF, 0xFF);
                } else {    
                    DEBUGWARN("currMode not erase, something wrong in flash!\n");
                }
                break;
            }
            case 0xA0: {
                // Prepare to write single data byte
                currMode = WRITE;
                break;
            }
            case 0xB0: {
                // Set memory bank
                currMode = MEM_BANK;
                break;
            }
        }
    }
}


uint8_t Flash::read(uint32_t address) {
    return flash[(address & 0x0000FFFF) + bank];
}

inline 
bool Flash::is4KbEraseCommand(uint32_t address, uint8_t value) {
    // TODO: correct?
    return (value == 0x30) && !((address & 0xFFFF0FFF) ^ 0x0E000000);
}

void Flash::setSize(uint32_t size) {
    assert(size == 512 || size == 1024);

    if(size == 1024) {
        manufacturerId = 0x62;
        deviceId = 0x13;
    } else {
        // size == 512
        manufacturerId = 0x32;
        deviceId = 0x1B;
    }
}