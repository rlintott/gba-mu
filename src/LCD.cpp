#include <SFML/Graphics.hpp>
#include "LCD.h"
// TODO: common include file for defines (like DEBUG)
#include "ARM7TDMI.h"



void LCD::initWindow() {
    DEBUG("hello\n");
    sf::RenderWindow gbaWindow(sf::VideoMode(800, 600), "GBA Window");

    while(gbaWindow.isOpen()) {
        sf::Event event;

        while(gbaWindow.pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                gbaWindow.close();
            }
        }

        gbaWindow.clear(sf::Color::Green);

        gbaWindow.display();

    }
}


