#include <cstdint>
#include <vector>

class Bus; 

class PPU {

    public:
        void renderScanline(uint16_t scanline);
        void renderObject();
        bool isObjectDirty();

        static const uint32_t H_VISIBLE_CYCLES = 960;
        static const uint32_t H_BLANK_CYCLES = 272;
        static const uint32_t H_TOTAL = 1232;

        static const uint32_t V_VISIBLE_CYCLES = 197120;
        static const uint32_t V_BLANK_CYCLES = 83776;

    private:
        Bus* bus; 
        std::vector<uint16_t> pixelBuffer;
};