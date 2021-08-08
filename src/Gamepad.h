#include <SFML/Graphics.hpp>

class Bus;

class Gamepad {

    public:
        static void getInput(Bus* bus);

        // TODO make configurable
        static const sf::Keyboard::Key A = sf::Keyboard::K;
        static const sf::Keyboard::Key B = sf::Keyboard::M;
        static const sf::Keyboard::Key DPAD_LEFT = sf::Keyboard::A;
        static const sf::Keyboard::Key DPAD_UP = sf::Keyboard::W;
        static const sf::Keyboard::Key DPAD_RIGHT = sf::Keyboard::D;
        static const sf::Keyboard::Key DPAD_DOWN = sf::Keyboard::S;
        static const sf::Keyboard::Key SHOULDER_RIGHT = sf::Keyboard::E;
        static const sf::Keyboard::Key SHOULDER_LEFT = sf::Keyboard::Q;
        static const sf::Keyboard::Key START = sf::Keyboard::Space;
        static const sf::Keyboard::Key SELECT = sf::Keyboard::RShift;

};