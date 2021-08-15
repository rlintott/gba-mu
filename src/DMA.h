#include <cstdint>

class Bus;

class DMA {

    public:
        void connectBus(Bus* bus);
        uint32_t step(bool hBlank, bool vBlank);

    private:
        Bus *bus;
        uint32_t dma(uint8_t x, bool vBlank, bool hBlank);

        static const uint32_t internalMemMask = 0x07FFFFFF;
        static const uint32_t anyMemMask      = 0x0FFFFFFF;
        static const uint16_t dma3MaxWordCount = 0x10000;
        static const uint16_t dma012MaxWordCount = 0x4000;


};