#include "PPU.h"
#include "memory/Bus.h"
#include <SFML/Graphics.hpp>
#include <utility>
#include <algorithm>
#include <string>
#include <iostream>
#include "util/macros.h"
#include "assert.h"
#include <cmath>


PPU::PPU() {

}

PPU::~PPU() {

}

void PPU::renderScanline(uint16_t scanline) {
    // TODO: explain this logic in a comment
    if(scanline > (SCREEN_HEIGHT - 2) && scanline < 226) {
        return;
    }
    
    if(scanline > (SCREEN_HEIGHT - 1)) {
        // for special case where scanline == 226
        scanlineBackDropColours[0] = getBackdropColour();
    } else {
        scanlineBackDropColours[(scanline + 1) % 228] = getBackdropColour();
    }

    uint8_t bgMode = (bus->iORegisters[Bus::DISPCNT] & 0x7);

    switch(bgMode) {
        case 0: {
            // render spritebuffer for [scanline+2, 159] 
            renderSprites((scanline + 2) % 228);
            // render bgbuffer for [scanline+1, 159]  
            renderBg((scanline + 1) % 228);
            break;
        }
        case 1: {
            DEBUGWARN("in bg mode 1 unimplemented\n");
            break;
        }
        case 2: {
            DEBUGWARN("in bg mode 2 unimplemented\n");
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
                bgBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * SCREEN_WIDTH + x) << 1) + 1] << 8) | 
                                                           (bus->vRam[(scanline * SCREEN_WIDTH + x) << 1]); 
            } 
            break;
        }
        case 4: {
            // page flipping mode
            renderSprites((scanline + 2) % 228);
            if(!(bus->iORegisters[Bus::IORegister::DISPCNT] & 0x10)) {
                // page 0
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    bgBuffer[scanline * SCREEN_WIDTH + x] = 
                        indexBgPalette8Bpp(bus->vRam[(scanline * SCREEN_WIDTH + x)]);
                } 
            } else {
                // page 1
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    bgBuffer[scanline * SCREEN_WIDTH + x] = 
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
                    if(scanline >= 128 || x >= 160) {
                        bgBuffer[scanline * SCREEN_WIDTH + x] = indexBgPalette8Bpp(0);
                    } else {
                        bgBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * 160 + x) << 1) + 1] << 8) | 
                                                                    (bus->vRam[(scanline * 160 + x) << 1]); 
                    }
                } 
            } else {
                // page 1
                for(int x = 0; x < SCREEN_WIDTH; x++) {
                    if(scanline >= 128 || x >= 160) {
                        bgBuffer[scanline * SCREEN_WIDTH + x] = indexBgPalette8Bpp(0);
                    } else {
                        bgBuffer[scanline * SCREEN_WIDTH + x] = (bus->vRam[((scanline * 160 + x) << 1) + 1 + 0xA000] << 8) | 
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
}

void PPU::connectBus(std::shared_ptr<Bus> _bus) {
    this->bus = _bus;
}


uint16_t PPU::getBackdropColour() {
    return ((uint16_t)bus->paletteRam[0 << 1]) |
           ((uint16_t)bus->paletteRam[(0 << 1) + 1] << 8);
}

uint32_t PPU::indexBgPalette8Bpp(uint8_t index) {
    if(!index) {
        return transparentColour;
    }
    return ((uint16_t)bus->paletteRam[index << 1]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1] << 8);
}

uint32_t PPU::indexBgPalette4Bpp(uint8_t index) {
    if(!(index & 0x0F)) {
        return transparentColour;
    }
    return ((uint16_t)bus->paletteRam[index << 1]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1] << 8);
}


uint32_t PPU::indexObjPalette4Bpp(uint8_t index) {
    // TODO: this is just temporary
    if(!(index & 0x0F)) {
        return transparentColour;
    }
    return ((uint16_t)bus->paletteRam[(index << 1) + 0x200]) |
           ((uint16_t)bus->paletteRam[(index << 1) + 1 + 0x200] << 8);    
}

uint32_t PPU::indexObjPalette8Bpp(uint8_t index) {
    if(!index) {
        return transparentColour;
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
// TODO: Maximum Number of Sprites per Line
void PPU::renderSprites(uint16_t scanline) {
    if(scanline > SCREEN_HEIGHT - 1) {
        return;
    }

    if(bus->iORegisters[Bus::IORegister::DISPCNT + 1] & 0x80) {
        // SPRITE WINDOW SPRITE WINDOW SPRITE WINDOW!!!!!
        scanlineObjectWindowData[scanline] = bus->iORegisters[Bus::IORegister::WINOUT + 1] & 0x3F;
    }

    //spriteBuffer.fill(transparentColour);
    // mapping mode 1 =  1d mapping, 0 = 2d mapping
    bool oneDimMapping = bus->iORegisters[Bus::IORegister::DISPCNT] & 0x40;
    // interate through all OAM attributes (object), from lowest priority to highest
    // TODO: get rid of the magic numbers
    for(int32_t address = 0x3F8; address >= 0x0; address -= 0x8) {
        uint16_t objAttr0 = bus->objAttributes[address] | (bus->objAttributes[address + 1] << 8);
        uint16_t objAttr1 = bus->objAttributes[address + 2] | (bus->objAttributes[address + 2 + 1] << 8);
        uint16_t objAttr2 = bus->objAttributes[address + 4] | (bus->objAttributes[address + 4 + 1] << 8);

        bool colorMode = objAttr0 & 0x2000; // 16 colors (4bpp) if cleared; 256 colors (8bpp) if set.

        uint8_t objMode = (objAttr0 & 0x0300) >> 8;
        if(objMode == 2) {
            // do not render this sprite
            continue;
        }

        // to be included in spriteBuffer pixel bits 16-17 for render calclation at frame draw time
        uint32_t drawMode = (uint32_t)(objAttr0 & 0x0C00) << 6;

        uint32_t base = (objAttr2 & 0x03FF);

        uint8_t paletteBank = (objAttr2 & 0xF000) >> 8;
        uint8_t priority = (objAttr2 & 0x0C00) >> 10;

        // [shape][size]
        Dimension dim = spriteDimensions[(objAttr0 & 0xC000) >> 14][(objAttr1 & 0xC000) >> 14];
        int32_t width = dim.width * 8;
        int32_t height = dim.height * 8;

        // implementation detail
        uint32_t priorityOffset = SCREEN_WIDTH * SCREEN_HEIGHT * priority;
        int32_t screenXOffset = objAttr1 & 0x01FF;
        int32_t screenYOffset = objAttr0 & 0x00FF;

        int16_t pa = 0x100;
        int16_t pb = 0;
        int16_t pc = 0;
        int16_t pd = 0x100;
        int32_t boundingWidth = width;
        int32_t boundingHeight = height;
        bool hFlip = false;
        bool vFlip = false;

        if(objAttr0 & 0x100) {
            // rotation / scaling flag (affine sprites enabled)
            if(objAttr0 & 0x200) {
                boundingWidth *= 2;
                boundingHeight *= 2;
            } 
            uint32_t paramSelection = (objAttr1 & 0x3E00) >> 9;
            uint32_t paramBaseAddr = 0x6 + paramSelection * 32;
            pa = bus->objAttributes[paramBaseAddr + 0 * 8] | (bus->objAttributes[paramBaseAddr + 0 * 8 + 1] << 8);
            pb = bus->objAttributes[paramBaseAddr + 1 * 8] | (bus->objAttributes[paramBaseAddr + 1 * 8 + 1] << 8);
            pc = bus->objAttributes[paramBaseAddr + 2 * 8] | (bus->objAttributes[paramBaseAddr + 2 * 8 + 1] << 8);
            pd = bus->objAttributes[paramBaseAddr + 3 * 8] | (bus->objAttributes[paramBaseAddr + 3 * 8 + 1] << 8);

        } else {
            if(objAttr0 & 0x200) {
                // obj disabled
                continue;
            } 

            if(objAttr1 & 0x1000) {
                // horizontal flip
                hFlip = true;
            }
            if(objAttr1 & 0x2000) {
                // vertical flip
                vFlip = true;
            }
        }

        int32_t halfWidth = boundingWidth / 2;
        int32_t halfHeight = boundingHeight / 2;

        if(screenYOffset >= SCREEN_HEIGHT) {
            screenYOffset = screenYOffset - 255;
        }

        int32_t y = (int32_t)scanline - screenYOffset - halfHeight;

        int32_t screenY = scanline;
    
        if((y + halfHeight) < 0 || boundingHeight < (y + halfHeight)) {
            // sprite outside scanline, doesnt need to be rendered
            continue;
        }

        for(int32_t x = -halfWidth; x < halfWidth; x++) {
            
            int32_t screenX = x + screenXOffset + halfWidth;

            screenX &= 0x1FF;

            if(screenX >= SCREEN_WIDTH) {
                continue;
            }
    
            uint32_t colour;
            uint32_t tileAddress = 0;

            int32_t textureX = ((pa * x + pb * y) >> 8) + (width / 2);
            int32_t textureY = ((pc * x + pd * y) >> 8) + (height / 2);

            if(hFlip) {
                textureX = width - textureX - 1;
            }
            if(vFlip) {
                textureY = height - textureY - 1;
            }

            if(textureX < 0 || width <= textureX ||  
               textureY < 0 || height <= textureY) {
                continue;
            }

            if(colorMode) {

                // 8 bpp
                uint32_t tileNum = oneDimMapping ? base + ((textureY / 8) * dim.width + (textureX / 8)) * 2 :
                            base + ((textureY / 8) * 32 + (textureX / 8)) * 2;

                tileNum &= 0x3FF;
                tileAddress = 0x10000 + tileNum * 0x20; 
            } else {

                // 4 bpp
                uint32_t tileNum = oneDimMapping ? base + ((textureY / 8) * dim.width + (textureX / 8)):
                            base + ((textureY / 8) * 32 + (textureX / 8));

                tileNum &= 0x3FF;
                tileAddress = 0x10000 + tileNum * 0x20; 
            }    


            if(colorMode) {
                colour = indexObjPalette8Bpp(bus->vRam[tileAddress + (textureY % 8) * 8 + (textureX % 8)]);
            } else {
                if(textureX % 2) {
                    colour = indexObjPalette4Bpp(((bus->vRam[tileAddress + (((textureY % 8) * 8 + (textureX % 8)) >> 1)] & 0xF0) >> 4) | paletteBank); 
                } else {
                    colour = indexObjPalette4Bpp((bus->vRam[tileAddress + (((textureY % 8) * 8 + (textureX % 8)) >> 1)] & 0xF) | paletteBank);
                }
            }

            if(colour != transparentColour) {
                spriteBuffer[priorityOffset + screenY * SCREEN_WIDTH + screenX] = colour | drawMode;
            }
        
        }
    }

}


void PPU::renderBg(uint16_t scanline) {
    if(scanline > (SCREEN_HEIGHT - 1)) {
        scanline = 0;
    }

    if(bus->iORegisters[Bus::IORegister::DISPCNT + 1] & 0xE0) {
        // WINDOWING WINDOWING WINDOWING
        if(bus->iORegisters[Bus::IORegister::DISPCNT + 1] & 0x20) {
            // window 0
            scanlineBgWindowData[scanline].enabled = true;
            scanlineBgWindowData[scanline].bottom = bus->iORegisters[Bus::IORegister::WIN0V];
            scanlineBgWindowData[scanline].top = (bus->iORegisters[Bus::IORegister::WIN0V + 1]);
            scanlineBgWindowData[scanline].right = bus->iORegisters[Bus::IORegister::WIN0H];
            scanlineBgWindowData[scanline].left = bus->iORegisters[Bus::IORegister::WIN0H + 1];
            scanlineBgWindowData[scanline].metaData = bus->iORegisters[Bus::IORegister::WININ] & 0x3F;
        }
        if(bus->iORegisters[Bus::IORegister::DISPCNT + 1] & 0x40) {
            // window 1
            scanlineBgWindowData[SCREEN_HEIGHT + scanline].enabled = true;
            scanlineBgWindowData[SCREEN_HEIGHT + scanline].bottom = bus->iORegisters[Bus::IORegister::WIN1V];
            scanlineBgWindowData[SCREEN_HEIGHT + scanline].top = (bus->iORegisters[Bus::IORegister::WIN1V + 1]);
            scanlineBgWindowData[SCREEN_HEIGHT + scanline].right = bus->iORegisters[Bus::IORegister::WIN1H];
            scanlineBgWindowData[SCREEN_HEIGHT + scanline].left = bus->iORegisters[Bus::IORegister::WIN1H + 1];
            scanlineBgWindowData[SCREEN_HEIGHT + scanline].metaData = bus->iORegisters[Bus::IORegister::WININ + 1] & 0x3F;
        }  
        scanlineOutsideWindowData[scanline] = bus->iORegisters[Bus::IORegister::WINOUT] & 0x3F;
    }

    renderBgX(scanline, 0);
    renderBgX(scanline, 1);
    renderBgX(scanline, 2);
    renderBgX(scanline, 3);
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
    uint32_t bgBufferOffset = x * SCREEN_HEIGHT * SCREEN_WIDTH;
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

    uint32_t tileWidth = colorMode ? 64 : 32;

    int16_t pa = 1;
    int16_t pb = 0;
    int16_t pc = 1;
    int16_t pd = 0;

    for(uint8_t x = 0; x < width; x++) {
        uint8_t screenBlockX = x / 32;
        for(uint8_t y = 0; y < height; y++) {
            /*
                In 'Text Modes', the screen size is organized as follows: 
                The screen consists of one or more 256x256 pixel (32x32 tiles) areas. 
                When Size=0: only 1 area (SC0), 
                when Size=1 or Size=2: two areas (SC0,SC1 either horizontally or vertically arranged next to each other), 
                when Size=3: four areas (SC0,SC1 in upper row, SC2,SC3 in lower row). 
                Whereas SC0 is defined by the normal BG Map base address 
                (Bit 8-12 of BGxCNT), SC1 uses same address +2K, SC2 address +4K, SC3 address +6K. 
                When the screen is scrolled it'll always wraparound.
            */
            uint8_t screenBlockY = y / 32;
            uint8_t screenBlock = (screenBlockX + screenBlockY * (width / 32));

            uint32_t addr = (((y % 32) * 32) + (x % 32)) * 2 + (screenBaseBlock + screenBlock) * 0x800;
            
            uint16_t screenEntry = ((uint16_t)bus->vRam[addr]) | 
                                  ((uint16_t)(bus->vRam[addr + 1] << 8));


            int8_t hFlipMultiplier = 1;
            int8_t hFlipOffset = 0;
            int8_t vFlipMultiplier = 1;
            int8_t vFlipOffset = 0;

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

            // first check if this tile will be rendered
            screenY = ((uint32_t)(y * 8 + (vFlipOffset + vFlipMultiplier * 0)) - vOffset) % ((uint32_t)height * 8);
            if((screenY + 7) < scanline) {continue;}

            uint8_t paletteBank = (screenEntry & 0xF000) >> 8;

            uint32_t tileIndex = screenEntry & 0x3FF;
            uint32_t offset = ((uint32_t)tileBaseBlock) * 0x4000 + tileIndex * tileWidth; // TODO: or 64 if in 8bpp mode

            for(uint8_t tileY = 0; tileY < 8; tileY++) {
                screenY = (((uint32_t)y * 8 + (vFlipOffset + vFlipMultiplier * tileY)) - vOffset) % ((uint32_t)height * 8);

                if(screenY != scanline) {
                    continue;
                }
                for(uint8_t tileX = 0; tileX < 8; tileX++) {
                    screenX = (((uint32_t)x * 8 + (hFlipOffset + hFlipMultiplier * tileX)) - hOffset) % ((uint32_t)width * 8);
                    
                    if(screenX >= SCREEN_WIDTH || screenY >= SCREEN_HEIGHT) {
                        continue;
                    }
                    
                    if(colorMode) {
                        bgBuffer[bgBufferOffset + screenY * SCREEN_WIDTH + screenX] = 
                            indexBgPalette8Bpp(bus->vRam[offset + tileY * 8 + tileX]) | ((uint32_t)priority << 16);
                   } else {
                        if(tileX % 2) {
                            // tile x odd
                            // TODO: clean this up...
                            bgBuffer[bgBufferOffset + screenY * SCREEN_WIDTH + screenX] = 
                                indexBgPalette4Bpp(((bus->vRam[offset + ((tileY * 8 + tileX) / 2)] & 0xF0) >> 4) | paletteBank) | 
                                ((uint32_t)priority << 16);
                        } else {
                            // tile x even
                            bgBuffer[bgBufferOffset + screenY * SCREEN_WIDTH + screenX] = 
                                indexBgPalette4Bpp((bus->vRam[offset + ((tileY * 8 + tileX) / 2)] & 0xF) | paletteBank) |
                                ((uint32_t)priority << 16);
                        }                    
                    }

                }
                if(screenY == scanline) break;
            }
            if(screenY == scanline) break;           
        } 
    } 
}

bool PPU::isTransparent(uint32_t pixelData) {
    return pixelData & transparentColour;
}

PPU::Coords PPU::convertScreenCoordsToSpriteCoords(int32_t screenX, int32_t screenY, int16_t pa, int16_t pb, int16_t pc, int16_t pd, int32_t xRotCentre, int32_t yRotCentre) {

    // float paScale = (float)(pa) / 256.0;
    // float pbScale = (float)(pb) / 256.0;
    // float pcScale = (float)(pc) / 256.0;
    // float pdScale = (float)(pd) / 256.0;

    // uint32_t spriteX = ((paScale * (float)((int32_t)screenX - (int32_t)xRotCentre) + pbScale * (float)((int32_t)screenY - (int32_t)yRotCentre) + (float)(xRotCentre)));
    // uint32_t spriteY = ((pcScale * (float)((int32_t)screenX - (int32_t)xRotCentre) + pdScale * (float)((int32_t)screenY - (int32_t)yRotCentre) + (float)(yRotCentre)));
    int32_t spriteX = ((pa * (int)screenX + pb * (int)screenY) >> 8) + xRotCentre;
    int32_t spriteY = ((pc * (int)screenX + pd * (int)screenY) >> 8) + yRotCentre;
    // uint32_t spriteX = (paScale * (float)((int32_t)screenX ) + pbScale * (float)((int32_t)screenY) );
    // uint32_t spriteY = (pcScale * (float)((int32_t)screenX ) + pdScale * (float)((int32_t)screenY) );

    DEBUGWARN(spriteX << " spriteX\n");
    DEBUGWARN(spriteX << " spriteY\n");

    return {spriteX, spriteY};
}


// this is only called once per frame
std::array<uint16_t, PPU::SCREEN_WIDTH * PPU::SCREEN_HEIGHT>& PPU::renderCurrentScreen() {
    pixelBuffer.fill(0);
    // get the priorities of the backgrounds
    std::vector<std::pair<uint8_t, uint8_t>> bgPriorities;
    bgPriorities.push_back({(bgBuffer[0 * SCREEN_WIDTH * SCREEN_HEIGHT] & 0x30000) >> 16, 0});
    bgPriorities.push_back({(bgBuffer[1 * SCREEN_WIDTH * SCREEN_HEIGHT] & 0x30000) >> 16, 1});
    bgPriorities.push_back({(bgBuffer[2 * SCREEN_WIDTH * SCREEN_HEIGHT] & 0x30000) >> 16, 2});
    bgPriorities.push_back({(bgBuffer[3 * SCREEN_WIDTH * SCREEN_HEIGHT] & 0x30000) >> 16, 3});
    std::sort(bgPriorities.begin(), bgPriorities.end());
    // because going from lowest (3) to highest (0) prioirty
    // In case that the 'Priority relative to BG' is the same than the priority of one of the background layers, 
    // then the OBJ becomes higher priority and is displayed on top of that BG layer.   

    bool win0Enabled = false;
    bool win1Enabled = false;


    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        uint8_t windowBgMask;
        uint8_t window0Left = 0;
        uint8_t window0Right = 0;
        uint8_t window1Left = 0;
        uint8_t window1Right = 0;
        bool window0 = false;
        bool window1 = false;
        bool windowed = false;
        if(scanlineBgWindowData[y].enabled) {
            windowed = true;
            // WINDOW 0
            if((scanlineBgWindowData[y].top <= y && y < scanlineBgWindowData[y].bottom) || 
               ((int8_t)scanlineBgWindowData[y].top <= y && y < (int8_t)scanlineBgWindowData[y].bottom)) {
                window0 = true;
                window0Left = scanlineBgWindowData[y].left;
                window0Right = scanlineBgWindowData[y].right;
            }
        }
        if(scanlineBgWindowData[SCREEN_HEIGHT + y].enabled) {
            windowed = true;
            // WINDOW 1
            if((scanlineBgWindowData[SCREEN_HEIGHT + y].top <= y && y < scanlineBgWindowData[SCREEN_HEIGHT + y].bottom) || 
               ((int8_t)scanlineBgWindowData[SCREEN_HEIGHT + y].top <= y && y < (int8_t)scanlineBgWindowData[SCREEN_HEIGHT + y].bottom)) {
                window1 = true;
                window1Left = scanlineBgWindowData[SCREEN_HEIGHT + y].left;
                window1Right = scanlineBgWindowData[SCREEN_HEIGHT + y].right;
            }         
        }

        for(int x = 0; x < SCREEN_WIDTH; x++) {
            pixelBuffer[y * SCREEN_WIDTH + x] = scanlineBackDropColours[y];

            for(int priority = 3; priority >= 0; priority--) {
                uint32_t bgOffset = (bgPriorities[priority].second) * SCREEN_HEIGHT * SCREEN_WIDTH;
                uint32_t bgPixel = bgBuffer[bgOffset + y * SCREEN_WIDTH + x];
                int spriteRelativePrio = bgPriorities[priority].first;

                if(windowed) {
                    if(window0 && 
                       ((window0Left <= x && x < window0Right) || 
                        ((int8_t)window0Left <= (int8_t)x && (int8_t)x < (int8_t)window0Right))) {
                        windowBgMask = scanlineBgWindowData[y].metaData; 
                    }   
                    else if(window1 &&                        
                           ((window1Left <= x && x < window1Right) || 
                           ((int8_t)window1Left <= (int8_t)x && (int8_t)x < (int8_t)window1Right))) {
                        windowBgMask = scanlineBgWindowData[SCREEN_HEIGHT + y].metaData; 
                    }
                    else {
                        windowBgMask = scanlineOutsideWindowData[y]; 
                    }
                    if((windowBgMask & (1 << (bgPriorities[priority].second)))) {
                        if(!isTransparent(bgPixel)) {
                            pixelBuffer[y * SCREEN_WIDTH + x] = bgPixel & 0xFFFF;
                        }                        
                    } 
                    if(windowBgMask & 0x10) {
                        // obj enable
                        for(int spritePrio = spriteRelativePrio; spritePrio >= 0; spritePrio--) {
                            uint32_t spriteOffset = spritePrio * SCREEN_HEIGHT * SCREEN_WIDTH;
                            uint32_t spritePixel = spriteBuffer[spriteOffset + y * SCREEN_WIDTH + x];
                            if(!isTransparent(spritePixel)) {
                                pixelBuffer[y * SCREEN_WIDTH + x] = spritePixel & 0xFFFF;
                        
                            }
                        }
                    }
                    // TODO: sprite window

                } else {
                    if(!isTransparent(bgPixel)) {
                        pixelBuffer[y * SCREEN_WIDTH + x] = bgPixel & 0xFFFF;
                    } 
                    for(int spritePrio = spriteRelativePrio; spritePrio >= 0; spritePrio--) {
                        uint32_t spriteOffset = spritePrio * SCREEN_HEIGHT * SCREEN_WIDTH;
                        uint32_t spritePixel = spriteBuffer[spriteOffset + y * SCREEN_WIDTH + x];
                        if(!isTransparent(spritePixel)) {
                            pixelBuffer[y * SCREEN_WIDTH + x] = spritePixel & 0xFFFF;        
                        }
                    }
                }

            }

        }
    }
    bgBuffer.fill(transparentColour | lowestPrio);
    spriteBuffer.fill(transparentColour);
    for(auto& windowData : scanlineBgWindowData) {
        windowData.enabled = false;
    }

    return pixelBuffer;
}


