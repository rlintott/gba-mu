#include "Debugger.h"
#include "ARM7TDMI.h"
#include "Bus.h"




void Debugger::step() {
    printState();
    getInput();
}

void Debugger::printState() {
    printf(
        "\n\
        **********************************CPU STATE***********************************\n\
        r0:\t   0x00000000  r0:     0x00000000  r0:     0x00000000  r0:     0x00000000\n\  
        r0:     0x00000000  r0:     0x00000000  r0:     0x00000000  r0:     0x00000000\n\
        r0:     0x00000000  r0:     0x00000000  r0:     0x00000000  r0:     0x00000000\n\
        r0:     0x00000000  r0:     0x00000000  r0(SP): 0x00000000  r0(LR): 0x00000000\n\
        r0(PC): 0x00000000\n\
        \n\
        r12irq: 0x00000000  r12irq: 0x00000000\n\ 
        r12svc: 0x00000000  r12svc: 0x00000000\n\
        r12und: 0x00000000  r12und: 0x00000000\n\ 
        \n\
        flags:      N|Z|C|V|Q|J|E|A|I|F|T| MODE         [0xFFE34185](32):   0x1545FFEA\n\
        cpsr:       0|0|0|0|0|0|0|0|0|0|0|00001         [0xFFE34185](8):    0x15\n\
        spsr_svc:   0|0|0|0|0|0|0|0|0|0|0|00001         [0xFFE34185](8):    0x15\n\
        spsr_fiq:   0|0|0|0|0|0|0|0|0|0|0|00001         [0xFFE34185](16):   0x15FA\n\
        spsr_abt:   0|0|0|0|0|0|0|0|0|0|0|00001         [0xFFE34185](8):    0x15\n\
        spsr_irq:   0|0|0|0|0|0|0|0|0|0|0|00001         [0xFFE34185](8):    0x15\n\
        spsr_und:   0|0|0|0|0|0|0|0|0|0|0|00001         [----------](-):    -\n\
        \n\
        [0x08049158]: mov r0, r0 #2\n\
        ******************************************************************************\n\
        "
    );
}

void Debugger::getInput() {
    // printf("can enter memory address and width to watch, (ex. 0xFFFFFFFF 32):");
    // std::string input = "0";
    // std::future<std::string> future = std::async(getAnswer);

    // std::string result = future.get();
    // std::cout << "input was: " << result << "\n";
}


