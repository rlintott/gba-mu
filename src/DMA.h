#include <cstdint>

class Bus;
class ARM7TDMI;
class Scheduler;

class DMA {

    public:
        void connectBus(Bus* bus);
        void connectCpu(ARM7TDMI* cpu);
        void connectScheduler(Scheduler* scheduler);

        uint32_t dmaX(uint8_t x, bool vBlank, bool hBlank, uint16_t scanline);

        void updateDmaUponWrite(uint32_t address, uint32_t value, uint8_t width);

    private:
        Bus *bus;
        ARM7TDMI *cpu;
        Scheduler* scheduler;

        void scheduleDmaX(uint32_t x, uint8_t upperControlByte, bool immediately);

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