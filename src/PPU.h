#include <cstdint>
#include <vector>
#include <array>
#include <queue>

class Bus; 
class Scheduler;

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

        void connectBus(std::shared_ptr<Bus> bus);

        void updateOamState(uint32_t address, uint8_t value);

        void setObjectsDirty();

    private:
        std::shared_ptr<Bus> bus; 
        std::shared_ptr<Scheduler> scheduler;

        uint32_t indexBgPalette4Bpp(uint8_t index);
        uint32_t indexBgPalette8Bpp(uint8_t index);
        uint32_t indexObjPalette4Bpp(uint8_t index);        
        uint32_t indexObjPalette8Bpp(uint8_t index);  
        uint16_t getBackdropColour();   

        const uint32_t transparentColour = 0x00040000;
        bool isTransparent(uint32_t pixelData);

        // each element of array: bits 0-15: colour, bit 16-17: drawMode, bit 18: transparent,
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

        struct BgWindowData {
            bool enabled;
            uint8_t right;
            uint8_t left;
            uint8_t bottom;
            uint8_t top;
            /*
                Bit   Expl.
                0-3   Window x BG0-BG3 Enable Bits     (0=No Display, 1=Display)
                4     Window x OBJ Enable Bit          (0=No Display, 1=Display)
                5     Window x Color Special Effect    (0=Disable, 1=Enable)
            */
            uint8_t metaData;
        };

        // to find window y data at scanline x: [x * SCREEN_HEIGHT + y]
        std::array<BgWindowData, SCREEN_HEIGHT * 2> scanlineBgWindowData;
        /*
            Bit   Expl.
            0-3   Outside BG0-BG3 Enable Bits      (0=No Display, 1=Display)
            4     Outside OBJ Enable Bit           (0=No Display, 1=Display)
            5     Outside Color Special Effect     (0=Disable, 1=Enable)
        */
        std::array<uint8_t, SCREEN_HEIGHT> scanlineOutsideWindowData;

        /*
            Bit   Expl.
            8-11  OBJ Window BG0-BG3 Enable Bits   (0=No Display, 1=Display)
            12    OBJ Window OBJ Enable Bit        (0=No Display, 1=Display)
            13    OBJ Window Color Special Effect  (0=Disable, 1=Enable)
        */
        std::array<uint8_t, SCREEN_HEIGHT> scanlineObjectWindowData;

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