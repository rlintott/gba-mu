#include "Bus.h"
#include "Debugger.h"
#include "GameBoyAdvance.h"




int main() {
    Bus bus;
    ARM7TDMI cpu;

    GameBoyAdvance gba(&cpu, &bus);

    std::cout << "completed!" << "\n";
    return 0;
}
