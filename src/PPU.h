#include <cstdint>
#include <vector>

class Bus; 

class PPU {

    public:
        void renderScanline(uint16_t y);
        void renderObject();
        bool isObjectDirty();

    private:
        Bus* bus; 
        std::vector<uint16_t> pixelBuffer;

};