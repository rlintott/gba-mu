#include "PPU.h"
#include "Bus.h"
#include "assert.h"
#include <utility>
#include <algorithm>

PPU::PPU() {

}

PPU::~PPU() {

}

void PPU::renderScanline(uint16_t scanline) {
    if(scanline > (SCREEN_HEIGHT - 1) && scanline < 226) {
        return;
    }
    uint8_t bgMode = (bus->iORegisters[Bus::DISPCNT] & 0x7);

    switch(bgMode) {
        case 0: {
            if(dirty) {
                // render spriteBuffer for [scanline+2, 159]
                // std::fill(std::next(pixelBuffer.begin() + scanline + 2), 
                //           std::next(pixelBuffer.begin() + 38399), 
                //           pixelBuffer);
                pixelBuffer.fill(0);
                DEBUGWARN("render on scanline " << scanline << "\n");
                renderSprites((scanline + 2) % 228);
                // render bgbuffer for [scanline+1, 159]  
                renderBg(scanline + 1);
                dirty = false;
            }
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
            3  	    240	    160	    16	1x12C00h	No
            4 	    240	    160	    8	2x9600h	    Yes
            5 	    160	    128	    16	2xA000h	    Yes
        */
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
            // page flipping mode
            if(!(bus->iORegisters[Bus::IORegister::DISPCNT] & 0x10)) {
                // page 0
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    // DEBUGWARN("reading from vRam memory " << scanline * 480 + x << "\n");
                    if(scanline >= 128 || x >= 160) {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = indexBgPalette(0);
                    } else {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * 160 + x) << 1) + 1] << 8) | 
                                                                    (bus->vRam[(scanline * 160 + x) << 1]); 
                    }
                } 
            } else {
                // page 1
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    if(scanline >= 128 || x >= 160) {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = indexBgPalette(0);
                    } else {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * 160 + x) << 1) + 1 + 0xA000] << 8) | 
                                                                    (bus->vRam[((scanline * 160 + x) << 1) + 0xA000]); 
                    }                }   
            }
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
    // TODO: this is just temporary
    if(!(index & 0x0F)) {
        return 0;
    }
    return ((uint16_t)bus->paletteRam[(index << 1) + 0x200]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1 + 0x200] << 8);    
}

void PPU::setObjectsDirty() {
    dirty = true;
}

/*
    06010000-06017FFF  32 KBytes OBJ Tiles
*/
void PPU::renderSprites(uint16_t scanline) {
    bool mappingMode = bus->iORegisters[Bus::IORegister::DISPCNT] & 0x40;
    // interate through all OAM attributes (object), from lowest priority to highest
    // TODO: get rid of the magic numbers
    for(int32_t address = 0x3F8; address >= 0x0; address -= 0x8) {
        uint16_t objAttr0 = bus->objAttributes[address] | (bus->objAttributes[address + 1] << 8);
        uint16_t objAttr1 = bus->objAttributes[address + 2] | (bus->objAttributes[address + 2 + 1] << 8);
        uint16_t objAttr2 = bus->objAttributes[address + 4] | (bus->objAttributes[address + 4 + 1] << 8);

        uint8_t objMode = (objAttr0 & 0x0300) >> 8;
        if(objMode == 2) {
            // do not render this sprite
            continue;
        }

        uint32_t offset = 0x10000 + ((objAttr2 & 0x03FF)/* charName */ * 0x20); 
        uint8_t paletteBank = (objAttr2 & 0xF000) >> 8;
        bool colorMode = objAttr0 & 0x2000; // 16 colors (4bpp) if cleared; 256 colors (8bpp) if set.
        uint8_t priority = objAttr2 & 0x0C00 >> 10;

        // [shape][size]
        SpriteDimension dim = spriteDimensions[(objAttr0 & 0xC000) >> 14][(objAttr1 & 0xC000) >> 14];
        uint8_t width = dim.width;
        uint8_t height = dim.height;
        uint32_t priorityOffset = SCREEN_WIDTH * SCREEN_HEIGHT * priority;
        uint32_t screenX = 0;
        uint32_t screenY = 0;
        uint16_t screenXOffset = objAttr1 & 0x01FF;
        uint16_t screenYOffset = objAttr0 & 0x00FF;

        uint32_t vFlipOffset = 0;
        uint32_t hFlipOffset = 0;

        uint32_t vFlipMultiplier = 1;
        uint32_t hFlipMultiplier = 1;

        if(objAttr1 & 0x1000) {
            // horizontal flip
            hFlipOffset = width * 8 - 1;
            hFlipMultiplier = -1;
        }
        if(objAttr1 & 0x2000) {
            // vertical flip
            vFlipOffset = height * 8 - 1;
            vFlipMultiplier = -1;
        }

        for(uint32_t x = 0; x < width; x++) {
            for(uint32_t y = 0; y < height; y++) {
                // 4 bpp
                uint32_t tile = (y * width + x) * 32 + offset;
                for(uint8_t tileY = 0; tileY < 8; tileY++) {
                    screenY = vFlipOffset + vFlipMultiplier * (y * 8 + tileY) + screenYOffset & 0xFF;
                    if(screenY < scanline) {
                        continue;
                    }

                    for(uint8_t tileX = 0; tileX < 8; tileX++) {
                        screenX = hFlipOffset + hFlipMultiplier * (x * 8 + tileX) + screenXOffset & 0x1FF;
                        if(screenX >= SCREEN_WIDTH || screenY >= SCREEN_HEIGHT) {
                            continue;
                        }
                        if(colorMode) {
                            pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                indexObjPalette(bus->vRam[tile + tileY * 8 + tileX]);
                        } else {
                            if((tileY * 8 + tileX) % 2) {
                                pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                    indexObjPalette(((bus->vRam[tile + ((tileY * 8 + tileX) >> 1)] & 0xF0) >> 4) | paletteBank); 
                            } else {
                                pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                    indexObjPalette(bus->vRam[tile + ((tileY * 8 + tileX) >> 1)] & 0xF | paletteBank);
                            }
                        }
                    }
                }        
            }
        }
    }
}


/* 
    4000008h - BG0CNT - BG0 Control (R/W) (BG Modes 0,1 only)
    400000Ah - BG1CNT - BG1 Control (R/W) (BG Modes 0,1 only)
    400000Ch - BG2CNT - BG2 Control (R/W) (BG Modes 0,1,2 only)
    400000Eh - BG3CNT - BG3 Control (R/W) (BG Modes 0,2 only)
    Bit   Expl.
    0-1   BG Priority           (0-3, 0=Highest)
    2-3   Character Base Block  (0-3, in units of 16 KBytes) (=BG Tile Data)
    4-5   Not used (must be zero) (except in NDS mode: MSBs of char base)
    6     Mosaic                (0=Disable, 1=Enable)
    7     Colors/Palettes       (0=16/16, 1=256/1)
    8-12  Screen Base Block     (0-31, in units of 2 KBytes) (=BG Map Data)
    13    BG0/BG1: Not used (except in NDS mode: Ext Palette Slot for BG0/BG1)
    13    BG2/BG3: Display Area Overflow (0=Transparent, 1=Wraparound)
    14-15 Screen Size (0-3)
    Internal Screen Size (dots) and size of BG Map (bytes):
    Value  Text Mode      Rotation/Scaling Mode
    0      256x256 (2K)   128x128   (256 bytes)
    1      512x256 (4K)   256x256   (1K)
    2      256x512 (4K)   512x512   (4K)
    3      512x512 (8K)   1024x1024 (16K)
    In case that some or all BGs are set to same priority then BG0 is having the highest, and BG3 the lowest priority.
*/
void PPU::renderBg(uint16_t scanline) {



}