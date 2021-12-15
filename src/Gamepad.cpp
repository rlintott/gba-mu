#include "memory/Bus.h"
#include "Gamepad.h"

void Gamepad::getInput(Bus* bus) {
    /*
        Bit   Expl.
        0     Button A        (0=Pressed, 1=Released)
        1     Button B        (etc.)
        2     Select          (etc.)
        3     Start           (etc.)
        4     Right           (etc.)
        5     Left            (etc.)
        6     Up              (etc.)
        7     Down            (etc.)
        8     Button R        (etc.)
        9     Button L        (etc.)
        10-15 Not used
    */
    uint8_t keysPressedByte1 = 0;
    uint8_t keysPressedByte2 = 0;

    keysPressedByte1 |= !sf::Keyboard::isKeyPressed(A);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(B) << 1);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(SELECT) << 2);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(START) << 3);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(DPAD_RIGHT) << 4);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(DPAD_LEFT) << 5);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(DPAD_UP) << 6);
    keysPressedByte1 |= (!sf::Keyboard::isKeyPressed(DPAD_DOWN) << 7);
    keysPressedByte2 |= (!sf::Keyboard::isKeyPressed(SHOULDER_RIGHT) << 0);
    keysPressedByte2 |= (!sf::Keyboard::isKeyPressed(SHOULDER_LEFT) << 1);

    // if(sf::Keyboard::isKeyPressed(DPAD_LEFT)) {
    //     DEBUGWARN("left!\n");
    // }
    // if(sf::Keyboard::isKeyPressed(DPAD_RIGHT)) {
    //     DEBUGWARN("right!\n");
    // }
    // if(sf::Keyboard::isKeyPressed(DPAD_UP)) {
    //     DEBUGWARN("up!\n");
    // }
    // if(sf::Keyboard::isKeyPressed(DPAD_DOWN)) {
    //     DEBUGWARN("down!\n");
    // }

    bus->iORegisters[Bus::IORegister::KEYINPUT] = keysPressedByte1;
    bus->iORegisters[Bus::IORegister::KEYINPUT + 1] = keysPressedByte2;
}