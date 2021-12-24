#include <cstdint>
#include <array>


class EEPROM {


    public:
        void transferBitToEeprom(bool bit);
        uint32_t receiveBitFromEeprom();
        void startEeprom6BitRead();
        void startEeprom14BitRead();
        void startEeprom6BitRead();
        void startEeprom14BitRead();
        static constexpr uint32_t SIX_BIT_WRITE_SIZE = 73;
        static constexpr uint32_t FOURTEEN_BIT_WRITE_SIZE = 81;
        static constexpr uint32_t SIX_BIT_READ_SIZE = 9;
        static constexpr uint32_t FOURTEEN_BIT_READ_SIZE = 17;

    private:
        uint32_t address;
        uint32_t ithBit;
        uint32_t currAddressBit;
        uint32_t transferSize;
        uint32_t currValueBit;
        uint64_t valueToRead;
        uint64_t valueToWrite;
        bool readyToRead;
        bool isReading;
        bool isWriting;
        bool writeComplete;
        void reset();

        std::array<uint64_t, 0x400> eeprom;
};