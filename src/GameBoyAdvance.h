#include <string>

class ARM7TDMI;
class Bus;

class GameBoyAdvance {
   public:
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus);
    ~GameBoyAdvance();

    void loadRom(std::string path);
    void startRom();

   private:
    ARM7TDMI* arm7tdmi;
    Bus* bus;
};