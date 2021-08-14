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
    // TODO: explain this logic in a comment
    if(scanline > (SCREEN_HEIGHT - 2) && scanline < 226) {
        return;
    }
    //DEBUGWARN("start of render\n");

    uint8_t bgMode = (bus->iORegisters[Bus::DISPCNT] & 0x7);

    switch(bgMode) {
        case 0: {
            if(dirty) {
                pixelBuffer.fill(0);
                // render spritebuffer for [scanline+2, 159]  
                renderSprites((scanline + 2) % 228);
                // render bgbuffer for [scanline+1, 159]  
                renderBg((scanline + 1) % 228);
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
                        indexBgPalette8Bpp(bus->vRam[(scanline * SCREEN_WIDTH + x)]);
                } 
            } else {
                // page 1
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    // DEBUGWARN("reading from vRam memory " << scanline * 480 + x << "\n");
                    pixelBuffer[scanline * SCREEN_WIDTH + x] = 
                        indexBgPalette8Bpp(bus->vRam[(scanline * SCREEN_WIDTH + x + 0xA000)]);
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
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = indexBgPalette8Bpp(0);
                    } else {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * 160 + x) << 1) + 1] << 8) | 
                                                                    (bus->vRam[(scanline * 160 + x) << 1]); 
                    }
                } 
            } else {
                // page 1
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    if(scanline >= 128 || x >= 160) {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = indexBgPalette8Bpp(0);
                    } else {
                        pixelBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * 160 + x) << 1) + 1 + 0xA000] << 8) | 
                                                                    (bus->vRam[((scanline * 160 + x) << 1) + 0xA000]); 
                    }                
                }   
            }
            break;
        }
        default: {
            assert(false);
            break;
        }

    }
    // DEBUGWARN("end of render\n");

}

void PPU::connectBus(Bus* _bus) {
    this->bus = _bus;
}


uint16_t PPU::indexBgPalette8Bpp(uint8_t index) {
    return ((uint16_t)bus->paletteRam[index << 1]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1] << 8);
}

uint16_t PPU::indexBgPalette4Bpp(uint8_t index) {
    return ((uint16_t)bus->paletteRam[index << 1]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1] << 8);
}


uint16_t PPU::indexObjPalette4Bpp(uint8_t index) {
    // TODO: this is just temporary
    if(!(index & 0x0F)) {
        return 0;
    }
    return ((uint16_t)bus->paletteRam[(index << 1) + 0x200]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1 + 0x200] << 8);    
}

uint16_t PPU::indexObjPalette8Bpp(uint8_t index) {
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
    if(scanline >= SCREEN_HEIGHT - 2) {
        return;
    }
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
        Dimension dim = spriteDimensions[(objAttr0 & 0xC000) >> 14][(objAttr1 & 0xC000) >> 14];
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
                uint32_t tile = mappingMode ? (y * width + x) * 32 + offset :
                                              (y * 32 + x) * 32 + offset;
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
                                indexObjPalette8Bpp(bus->vRam[tile + tileY * 8 + tileX]);
                        } else {
                            if((tileY * 8 + tileX) % 2) {
                                pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                    indexObjPalette4Bpp(((bus->vRam[tile + ((tileY * 8 + tileX) >> 1)] & 0xF0) >> 4) | paletteBank); 
                            } else {
                                pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                    indexObjPalette4Bpp(bus->vRam[tile + ((tileY * 8 + tileX) >> 1)] & 0xF | paletteBank);
                            }
                        }
                    }
                }        
            }
        }
    }
}


void PPU::renderBg(uint16_t scanline) {
    if(scanline > (SCREEN_HEIGHT - 1)) {
        // for edge case where scanline == 226
        renderBgX(0, 0);
        renderBgX(0, 1);
        renderBgX(0, 2);
        renderBgX(0, 3);
    } else {
        renderBgX(scanline, 0);
        renderBgX(scanline, 1);
        renderBgX(scanline, 2);
        renderBgX(scanline, 3);
    }
}


/*
  BGXCNT
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
*/
void PPU::renderBgX(uint16_t scanline, uint8_t x) {
    if(!(bus->iORegisters[Bus::IORegister::DISPCNT + 1] & (1 << x))) {
        // check if x screen enabled
        return;
    }

    uint16_t bgCnt = bus->iORegisters[0x8 + x * 2] | (bus->iORegisters[0x8 + x * 2 + 1] << 8);
    uint8_t priority = bgCnt & 0x3;
    uint8_t tileBaseBlock = (bgCnt & 0xC) >> 2;
    uint8_t screenBaseBlock = (bgCnt & 0x1F00) >> 8;
    uint8_t size = (bgCnt & 0xC000) >> 14;
    Dimension bgDim = textBgDimensions[size];
    uint8_t width = bgDim.width;
    uint8_t height = bgDim.height; 

    uint16_t hOffset =  (bus->iORegisters[0x10 + x * 4] | 
                        (bus->iORegisters[0x10 + x * 4 + 1] << 8)) & 0x1FF;
    uint16_t vOffset =  (bus->iORegisters[0x12 + x * 4] | 
                        (bus->iORegisters[0x12 + x * 4 + 1] << 8)) & 0x1FF;

    uint32_t screenX = 0;
    uint32_t screenY = 0;

    bool colorMode = bgCnt & 0x0080;

    for(uint8_t x = 0; x < width; x++) {
        uint8_t screenBlockX = x / 32;
        for(uint8_t y = 0; y < height; y++) {
            uint8_t screenBlockY = y / 32;
            uint8_t screenBlock = screenBlockX + screenBlockY * width / 32;
            uint32_t addr = (((y % 32) * 32) + (x % 32)) * 2 + screenBaseBlock * 0x800 + screenBlock * 0x800;
            uint16_t screenEntry = bus->vRam[addr] | 
                                  (bus->vRam[addr + 1] << 8);

            uint8_t paletteBank = (screenEntry & 0xF000) >> 8;
            uint32_t tileIndex = screenEntry & 0x3FF;
            uint32_t offset = tileBaseBlock * 0x4000 + tileIndex * 32; // TODO: or 64 if in 8bpp mode

            uint8_t hFlipMultiplier = 1;
            uint8_t hFlipOffset = 0;
            uint8_t vFlipMultiplier = 1;
            uint8_t vFlipOffset = 0;

            if(screenEntry & 0x0400) {
                // hFlip
                hFlipMultiplier = -1;
                hFlipOffset = 7;
            }
            if(screenEntry & 0x0800) {
                // vFlip
                vFlipMultiplier = -1;
                vFlipOffset = 7;
            }

            for(uint8_t tileY = 0; tileY < 8; tileY++) {
                screenY = ((uint32_t)(y * 8 + (vFlipOffset + vFlipMultiplier * tileY)) - vOffset) % (height * 8);
                if(screenY < scanline) {
                    continue;
                }
                for(uint8_t tileX = 0; tileX < 8; tileX++) {
                    screenX = ((uint32_t)(x * 8 + (hFlipOffset + hFlipMultiplier * tileX)) - hOffset) % (width * 8);
                    if(screenX >= SCREEN_WIDTH || screenY >= SCREEN_HEIGHT) {
                        continue;
                    }
                    
                    if(colorMode) {
                        pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                            indexBgPalette8Bpp(bus->vRam[offset + tileY * 8 + tileX]);
                    } else {
                        if((tileY * 8 + tileX) % 2) {
                            pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                indexBgPalette4Bpp(((bus->vRam[offset + ((tileY * 8 + tileX) / 2)] & 0xF0) >> 4) | paletteBank); 
                        } else {
                            pixelBuffer[screenY * SCREEN_WIDTH + screenX] = 
                                indexBgPalette4Bpp(bus->vRam[offset + ((tileY * 8 + tileX) / 2)] & 0xF | paletteBank);
                        }                    
                    }
                }
            }           
        }      
    } 
}