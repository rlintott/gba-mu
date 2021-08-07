#include "PPU.h"
#include "Bus.h"
#include "assert.h"


PPU::PPU() {

}

PPU::~PPU() {

}

void PPU::renderScanline(uint16_t scanline) {
    if(scanline > 159) {
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
        3 ie 2 	    240	    160	    16	1x12C00h	No
        4 ie 3	    240	    160	    8	2x9600h	    Yes
        5 ie 4	    160	    128	    16	2xA000h	    Yes
        */
        case 2: {
            break;
        }
        case 3: {
            //DEBUGWARN("howdy\n");
            for(int x = 0; x < 240; x++) {
                // DEBUGWARN("reading from vRam memory " << scanline * 480 + x << "\n");
                pixelBuffer[scanline * 240 + x] = (bus->vRam[((scanline * 240 + x) * 2) + 1] << 8) | 
                                                         (bus->vRam[(scanline * 240 + x) * 2]); 

            } 
            break;
        }
        case 4: {
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