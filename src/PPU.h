#include <cstdint>
#include <vector>
#include <array>

class Bus; 

class PPU {

    public:
        PPU();
        ~PPU();

        void renderScanline(uint16_t scanline);
        void renderObject();
        bool isObjectDirty();

        static const uint32_t H_VISIBLE_CYCLES = 960;
        static const uint32_t H_BLANK_CYCLES = 272;
        static const uint32_t H_TOTAL = 1232;

        static const uint32_t V_VISIBLE_CYCLES = 197120;
        static const uint32_t V_BLANK_CYCLES = 83776;
        static const uint32_t V_TOTAL = 280896;
        
        static const uint32_t SCREEN_WIDTH = 240;
        static const uint32_t SCREEN_HEIGHT = 160;

        std::array<uint16_t, 38400> pixelBuffer = {};

        void connectBus(Bus* bus);

    private:
        Bus* bus; 
};