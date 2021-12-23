#include "LCD.h"
// TODO: common include file for defines (like DEBUG)
#include "arm7tdmi/ARM7TDMI.h"
#include "PPU.h"

/**
 * Helper function for changing resolution
 * TODO: move this somewere else
 * */
void changeResolution(sf::VertexArray& pixels, float xRes, float yRes) {
    float aspectRatio = (float)(PPU::SCREEN_WIDTH) / (float)(PPU::SCREEN_HEIGHT); // 1.5
    float newAspectRatio = (xRes / yRes);

    float xScale = xRes / PPU::SCREEN_WIDTH;
    float yScale = yRes / PPU::SCREEN_HEIGHT;

    if(newAspectRatio <= aspectRatio) {
        // xRes is limiting scale factor
        float yShift = yRes/2.0 - (PPU::SCREEN_HEIGHT * xScale / 2.0);
        for(int x = 0; x < PPU::SCREEN_WIDTH; x++) {
            for(int y = 0; y < PPU::SCREEN_HEIGHT; y++) {  
                sf::Vertex* quad = &pixels[(y * PPU::SCREEN_WIDTH + x) * 4];
                quad[0].position = sf::Vector2f(x * xScale, y * xScale + yShift);
                quad[1].position = sf::Vector2f((x + 1) * xScale, y * xScale + yShift);
                quad[2].position = sf::Vector2f((x + 1) * xScale, (y + 1) * xScale + yShift);
                quad[3].position = sf::Vector2f(x * xScale, (y + 1) * xScale + yShift);
            }
        }
    } else {
        float xShift = xRes/2.0 - (PPU::SCREEN_WIDTH * yScale / 2.0);
        for(int x = 0; x < PPU::SCREEN_WIDTH; x++) {
            for(int y = 0; y < PPU::SCREEN_HEIGHT; y++) {  
                sf::Vertex* quad = &pixels[(y * PPU::SCREEN_WIDTH + x) * 4];
                quad[0].position = sf::Vector2f(x * yScale + xShift, y * yScale);
                quad[1].position = sf::Vector2f((x + 1) * yScale + xShift, y * yScale);
                quad[2].position = sf::Vector2f((x + 1) * yScale + xShift, (y + 1) * yScale);
                quad[3].position = sf::Vector2f(x * yScale + xShift, (y + 1) * yScale);
            }
        }
    }
}

void LCD::initWindow() {
    gbaWindow = std::make_shared<sf::RenderWindow>(sf::VideoMode(PPU::SCREEN_WIDTH * defaultScreenSize, 
                                                   PPU::SCREEN_HEIGHT * defaultScreenSize), 
                                                   "gba-mu");
    pixels.resize(PPU::SCREEN_WIDTH * PPU::SCREEN_HEIGHT * 4);
    pixels.setPrimitiveType(sf::Quads);

    for(int x = 0; x < PPU::SCREEN_WIDTH; x++) {
        for(int y = 0; y < PPU::SCREEN_HEIGHT; y++) {  
            sf::Vertex* quad = &pixels[(y * PPU::SCREEN_WIDTH + x) * 4];
            sf::Color color = sf::Color(rand());
            quad[0].color = color;
            quad[1].color = color;
            quad[2].color = color;
            quad[3].color = color;

            quad[0].position = sf::Vector2f(x, y);
            quad[1].position = sf::Vector2f(x + 1, y);
            quad[2].position = sf::Vector2f(x + 1, y + 1);
            quad[3].position = sf::Vector2f(x, y + 1);
        }
    }

    sf::FloatRect visibleArea(0, 0, PPU::SCREEN_WIDTH * defaultScreenSize, 
                                    PPU::SCREEN_HEIGHT * defaultScreenSize);

    sf::View view = sf::View(visibleArea);
    gbaWindow->setView(view);
    changeResolution(pixels, (float)(PPU::SCREEN_WIDTH * defaultScreenSize), 
                             (float)(PPU::SCREEN_HEIGHT * defaultScreenSize));

    gbaWindow->clear(sf::Color::Black);
    gbaWindow->display();
}

/*
  0-4   Red Intensity   (0-31)
  5-9   Green Intensity (0-31)
  10-14 Blue Intensity  (0-31)
*/

void LCD::drawWindow(std::array<uint16_t, 38400>& pixelBuffer) {

    for(int i = 0; i < (pixelBuffer.size() * 4); i += 4) {
        uint16_t val = pixelBuffer[i >> 2];
        sf::Color colour = sf::Color((val & 0x1f) << 3, /* R */
                                     (val & 0x3E0) >> 2,  /* G */
                                     (val & 0x7C00) >> 7, 255); /* B */
        pixels[i].color = colour;
        pixels[i + 1].color = colour;
        pixels[i + 2].color = colour;
        pixels[i + 3].color = colour;
    }    

    if(gbaWindow->isOpen()) {
        while(gbaWindow->pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                gbaWindow->close();
                exit(0);
            }
            if(event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                sf::View view = sf::View(visibleArea);
                gbaWindow->setView(view);
                changeResolution(pixels, (float)event.size.width, (float)event.size.height);
            }
        }
        gbaWindow->clear(sf::Color::Black);
        gbaWindow->draw(pixels);
        
        gbaWindow->display();
    }
    
}


void LCD::closeWindow() {
    gbaWindow->close();
}

