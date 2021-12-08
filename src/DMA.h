#include <cstdint>

class Bus;
class ARM7TDMI;
class Scheduler;

class DMA {

    public:
        void connectBus(Bus* bus);
        void connectCpu(ARM7TDMI* cpu);

        // return 0 if DMA did not occur, else return # of cycles DMA took
        uint32_t step(bool hBlank, bool vBlank, uint16_t scanline);

        void updateDma(uint32_t ioReg, uint8_t newValue);

    private:
        Bus *bus;
        ARM7TDMI *cpu;
        Scheduler* scheduler;

        uint32_t dma(uint8_t x, bool vBlank, bool hBlank, uint16_t scanline);
        void modifyDmaX(uint32_t x, uint32_t ioReg, uint8_t newValue);

        static const uint32_t internalMemMask = 0x07FFFFFF;
        static const uint32_t anyMemMask      = 0x0FFFFFFF;
        static const uint32_t dma3MaxWordCount = 0x10000;
        static const uint32_t dma012MaxWordCount = 0x4000;

        bool dmaXEnabled[4] = {false, false, false, false};

        uint32_t dmaXSourceAddr[4] = {0, 0, 0, 0};
        uint32_t dmaXDestAddr[4] = {0, 0, 0, 0};
        uint32_t dmaXWordCount[4] = {0, 0, 0, 0};

        bool inVideoCaptureMode = false;

};