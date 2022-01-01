#pragma once
#include <cstdint>
#include <array>

class Flash {

    public: 
        enum Mode {
            READY,
            CHIP_ID,
            ERASE,
            WRITE,
            MEM_BANK
        };

        void write(uint32_t address, uint8_t value);
        uint8_t read(uint32_t address);

        void setSize(uint32_t size);


    private:
        Mode currMode = READY;
        int currStage = 0;
        std::array<uint8_t, 0x20000> flash;
        // different ids for 64k and 128k
        uint8_t manufacturerId = 0x62;
        uint8_t deviceId = 0x13;
        uint8_t temp0x0 = 0xFF;
        uint8_t temp0x1 = 0xFF;
        uint32_t bank = 0x0;

        bool is4KbEraseCommand(uint32_t address, uint8_t value);

};
