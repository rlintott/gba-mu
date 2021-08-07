#include "LCD.h"
// TODO: common include file for defines (like DEBUG)
#include "ARM7TDMI.h"

/**
 * Helper function for changing resolution
 * TODO: move this somewere else
 * */
void changeResolution(sf::VertexArray& pixels, float xRes, float yRes) {
    float aspectRatio = 240.0 / 160.0; // 1.5
    float newAspectRatio = xRes / yRes;

    float xScale = xRes / 240.0;
    float yScale = yRes / 160.0;

    DEBUG(newAspectRatio << "\n");
    if(newAspectRatio <= aspectRatio) {
        // xRes is limiting scale factor
        float yShift = yRes/2.0 - (160.0 * xScale / 2.0);
        for(int x = 0; x < 240; x++) {
            for(int y = 0; y < 160; y++) {  
                sf::Vertex* quad = &pixels[(y * 240 + x) * 4];
                quad[0].position = sf::Vector2f(x * xScale, y * xScale + yShift);
                quad[1].position = sf::Vector2f((x + 1) * xScale, y * xScale + yShift);
                quad[2].position = sf::Vector2f((x + 1) * xScale, (y + 1) * xScale + yShift);
                quad[3].position = sf::Vector2f(x * xScale, (y + 1) * xScale + yShift);
            }
        }
    } else {
        float xShift = xRes/2.0 - (240.0 * yScale / 2.0);
        for(int x = 0; x < 240; x++) {
            for(int y = 0; y < 160; y++) {  
                sf::Vertex* quad = &pixels[(y * 240 + x) * 4];
                quad[0].position = sf::Vector2f(x * yScale + xShift, y * yScale);
                quad[1].position = sf::Vector2f((x + 1) * yScale + xShift, y * yScale);
                quad[2].position = sf::Vector2f((x + 1) * yScale + xShift, (y + 1) * yScale);
                quad[3].position = sf::Vector2f(x * yScale + xShift, (y + 1) * yScale);
            }
        }
    }
}

void LCD::initWindow() {
    gbaWindow = new sf::RenderWindow(sf::VideoMode(240, 160), "GBA Window");
    // TODO: make 240, 160 constant member of lcd class
    pixels.resize(240 * 160 * 4);
    pixels.setPrimitiveType(sf::Quads);
    
    for(int x = 0; x < 240; x++) {
        for(int y = 0; y < 160; y++) {  
            sf::Vertex* quad = &pixels[(y * 240 + x) * 4];
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
    gbaWindow->clear(sf::Color::Black);
    gbaWindow->display();

}


void LCD::drawWindow(std::array<uint16_t, 38400>& pixelBuffer) {
    for(int i = 0; i < (pixelBuffer.size() * 4); i += 4) {
        uint16_t val = pixelBuffer[i >> 2];
        sf::Color colour = sf::Color((val & 0x1f) << 3, (val & 0x3E0) >> 2, (val & 0x7C00) >> 7, 255);
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

