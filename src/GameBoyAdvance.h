#include <string>

class ARM7TDMI;
class Bus;
class LCD;
class PPU;
class DMA;

class GameBoyAdvance {
   public:
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus, LCD* _screen, PPU* _ppu, DMA* _dma);
    GameBoyAdvance(ARM7TDMI* _arm7tdmi, Bus* _bus);
    ~GameBoyAdvance();

    bool loadRom(std::string path);
    void startRom();
    void loop();

    void testDisplay();

   private:
    ARM7TDMI* arm7tdmi;
    Bus* bus;
    LCD* screen;
    PPU* ppu;
    DMA* dma;

    bool hBlank = false;
    bool scanlineRendered = false;
    bool vBlank = false;

    long previousTime = 0;
    long currentTime = 0;
    long frames = 0;
    double startTimeSeconds = 0.0;
    

};