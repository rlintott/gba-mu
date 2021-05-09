#include "ARM7TDMI.h"
#include <iostream>


int main() {

    ARM7TDMI cpu;

    std::cout << "hello world!" << "\n";
    cpu.executeInstructionCycle();
    return 0;

}
