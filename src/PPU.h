#include <cstdint>
#include <vector>
#include <array>
#include <queue>

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

        std::array<uint16_t, SCREEN_WIDTH * SCREEN_HEIGHT>& renderCurrentScreen();

        std::array<uint16_t, SCREEN_WIDTH * SCREEN_HEIGHT> pixelBuffer = {};

        void connectBus(Bus* bus);

        void updateOamState(uint32_t address, uint8_t value);

        void setObjectsDirty();

    private:
        Bus* bus; 

        uint32_t indexBgPalette4Bpp(uint8_t index);
        uint32_t indexBgPalette8Bpp(uint8_t index);
        uint32_t indexObjPalette4Bpp(uint8_t index);        
        uint32_t indexObjPalette8Bpp(uint8_t index);  
        uint16_t getBackdropColour();   

        const uint32_t transparentColour = 0x00040000;
        bool isTransparent(uint32_t pixelData);

        // each element of array: bits 0-15: colour, bit 18: transparent
        // to find sprite pixel of priority i at location (x,y) -> [i * SCREEN_WIDTH * SCREEN_HEIGHT + y * SCREEN_WIDTH + x]
        std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT * 4> spriteBuffer = {};

        // each element of array: bits 0-15: colour, bits 16-17: priority, bit 18: transparent
        // to find pixel of bg#i at location (x,y) -> [i * SCREEN_WIDTH * SCREEN_HEIGHT + y * SCREEN_WIDTH + x]
        std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT * 4> bgBuffer = {};

        // backdrop colour for each scanline
        std::array<uint16_t, SCREEN_HEIGHT> scanlineBackDropColours;


        bool dirty;

        void renderSprites(uint16_t scanline);
        void renderBg(uint16_t scanline);
        void renderBgX(uint16_t scanline, uint8_t x);

        struct Dimension {
            uint8_t width;
            uint8_t height;
        };

        // in TILES, not pixels
        // [shape][size]
        Dimension spriteDimensions[3][4] = {
            { {1,1}, {2,2}, {4,4}, {8,8} },
            { {2,1}, {4,1}, {4,2}, {8,4} },
            { {1,2}, {1,4}, {2,4}, {4,8} }
        };

        Dimension textBgDimensions[4] = {
            {32, 32},
            {64, 32},
            {32, 64},
            {64, 64}
        };
};