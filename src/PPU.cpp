#include "PPU.h"
#include "Bus.h"
#include "assert.h"

void PPU::renderScanline(uint16_t scanline) {
    uint8_t bgMode;

    switch(bgMode) {
        case 0: {
            break;
        }
        case 1: {
            break;
        }
        case 2: {
            break;
        }
        /*
        mode	width	height	bpp	size	    page-flip
        3	    240	    160	    16	1x12C00h	No
        4	    240	    160	    8	2x9600h	    Yes
        5	    160	    128	    16	2xA000h	    Yes
        */
        case 3: {
            for(int x = 0; x < 480; x += 2) {
                pixelBuffer[scanline * 240 + x] = (bus->vRam[scanline * 240 + x + 1] << 8) | 
                                                   bus->vRam[scanline * 240 + x]; 
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