#pragma once

#include <cstdint>
#include <array>


class EEPROM {

    public:
        void transferBitToEeprom(bool bit);
        uint32_t receiveBitFromEeprom();
        static constexpr uint32_t SIX_BIT_WRITE_SIZE = 73;
        static constexpr uint32_t FOURTEEN_BIT_WRITE_SIZE = 81;
        static constexpr uint32_t SIX_BIT_READ_SIZE = 9;
        static constexpr uint32_t FOURTEEN_BIT_READ_SIZE = 17;
        enum {
            READ_OP = 3,
            WRITE_OP = 1
        };

    private:
        
        static constexpr uint32_t busWidth = 14;
        long currTransferBit = 0;
        long currReceivingBit = 0;
        uint32_t address;
        int currAddressBit;
        uint32_t currTransferSize;
        int currWriteValueBit;
        uint32_t currReadValueBit = 63;
        uint64_t valueToRead;
        uint64_t valueToWrite;
        bool readyToRead;
        bool writeComplete;
        bool firstBit;
        uint8_t op; // 2 = write, 3 = read TODO: ENUM

        std::array<uint64_t, 0x400> eeprom;
};