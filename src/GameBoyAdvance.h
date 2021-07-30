#include <string>

class ARM7TDMI;
class Bus;
class LCD;

class GameBoyAdvance {
   public:
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen);
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus);
    ~GameBoyAdvance();

    void loadRom(std::string path);
    void startRom();

    void testDisplay();

   private:
    ARM7TDMI* arm7tdmi;
    Bus* bus;
    LCD* screen;
};