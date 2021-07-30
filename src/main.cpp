#include "Bus.h"
#include "Debugger.h"
#include "GameBoyAdvance.h"
#include "LCD.h"




int main() {
    Bus bus;
    ARM7TDMI cpu;
    LCD screen;

    GameBoyAdvance gba(&cpu, &bus, &screen);

    gba.testDisplay();

    std::cout << "completed!" << "\n";
    return 0;
}
