#include "PPU.h"
#include "Bus.h"
#include "assert.h"


PPU::PPU() {

}

PPU::~PPU() {

}

void PPU::renderScanline(uint16_t scanline) {
    if(scanline > SCREEN_HEIGHT - 1) {
        return;
    }
    uint8_t bgMode = (bus->iORegisters[Bus::DISPCNT] & 0x7);

    switch(bgMode) {
        case 0: {
            break;
        }
        case 1: {
            break;
        }
        /*
        mode	width	height	bpp	size	    page-flip
        3  	    240	    160	    16	1x12C00h	No
        4 	    240	    160	    8	2x9600h	    Yes
        5 	    160	    128	    16	2xA000h	    Yes
        */
        case 2: {
            break;
        }
        case 3: {
            // simple bitmap mode
            for(int x = 0; x < SCREEN_WIDTH; x++) {
                // DEBUGWARN("reading from vRam memory " << scanline * 480 + x << "\n");
                pixelBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * SCREEN_WIDTH + x) << 1) + 1] << 8) | 
                                                           (bus->vRam[(scanline * SCREEN_WIDTH + x) << 1]); 
            } 
            break;
        }
        case 4: {
            // page flipping mode
            if(!(bus->iORegisters[Bus::IORegister::DISPCNT] & 0x10)) {
                // page 0
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    // DEBUGWARN("reading from vRam memory " << scanline * 480 + x << "\n");
                    pixelBuffer[scanline * SCREEN_WIDTH + x] = 
                        indexBgPalette(bus->vRam[(scanline * SCREEN_WIDTH + x)]);
                } 
            } else {
                // page 1
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    // DEBUGWARN("reading from vRam memory " << scanline * 480 + x << "\n");
                    pixelBuffer[scanline * SCREEN_WIDTH + x] = 
                        indexBgPalette(bus->vRam[(scanline * SCREEN_WIDTH + x + 0xA000)]);
                }   
            }

            break;
        }
        case 5: {
            break;
        }
        default: {
            assert(false);
            break;
        }

    }

}

void PPU::connectBus(Bus* _bus) {
    this->bus = _bus;
}


uint16_t PPU::indexBgPalette(uint8_t index) {
    return ((uint16_t)bus->paletteRam[index << 1]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1] << 8);
}


uint16_t PPU::indexObjPalette(uint8_t index) {
    
}