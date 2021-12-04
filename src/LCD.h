#include <SFML/Graphics.hpp>
#include <vector>

class LCD {

    public: 
        void initWindow();
        void drawWindow(std::array<uint16_t, 38400 /* width x height */>& pixelBuffer);
        void closeWindow();

    private: 
        static void drawPixel();
        sf::RenderWindow* gbaWindow;
        sf::VertexArray pixels;
        sf::Event event;
        int defaultScreenSize = 7;

    
};