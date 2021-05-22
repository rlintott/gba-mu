#include "Bus.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <bitset>
#include "Debugger.h"
#include "GameBoyAdvance.h"




int main() {
    Bus bus;
    ARM7TDMI cpu;

    GameBoyAdvance gba(&cpu, &bus);

    std::cout << "completed!" << "\n";
    return 0;
}
