
#include "Debugger.h"
#include <bitset>
#include <sstream>
#include "ncurses.h"


Debugger::Debugger(ARM7TDMI * cpu) {
    this->cpu = cpu;
    cpu->addDebugger(this);
};

Debugger::~Debugger() {
    endwin();
};


void Debugger::startDebugger() {
    this->initDisplay(42);


}

void Debugger::disassembleDataProcessing(uint32_t instruction) {
    arminstr_t armInstr;
    armInstr.raw = instruction;
    uint8_t opcode = armInstr.parsed.DATA_PROCESSING.opcode;
    data_processing_t dataProc = armInstr.parsed.DATA_PROCESSING;

    std::stringstream output;
    output << opcodeToName[ARM7TDMI::AluOpcode(opcode)] << 
        (dataProc.s ? "S" : "") << "\t" << 
        "rd=" << dataProc.rd << " " <<
        "rn=" << dataProc.rn << " " <<
        "operand2=" << std::bitset<12>(dataProc.operand2).to_string() << std::endl;
    
    displayInstr(output.str());

}

void Debugger::disassembleMultiply(uint32_t instruction) {

}

void Debugger::disassembleMultiplyLong(uint32_t instruction) {

}

void Debugger::disassembleSingleDataSwap(uint32_t instruction) {

}


void Debugger::initDisplay(uint32_t thing) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    WINDOW *instrBox = newwin(LINES, COLS/2, 0, 0);
    WINDOW *regBox = newwin(LINES/2, COLS/2, 0, COLS/2);
    WINDOW *psrBox = newwin(LINES/2, COLS/2, LINES/2,  COLS/2);

    box(instrBox, '*', '*');
    waddstr(instrBox, "Welcome to Ryan's ARM7TDMI Emu Debugger!\n");
    box(regBox, '*', '*');
    waddstr(regBox, "Registers:\n");
    box(psrBox, '*', '*');
    waddstr(psrBox, "Program status registers:\n");

    wrefresh(instrBox);
    wrefresh(regBox);
    wrefresh(psrBox);

    instrWindow = newwin(LINES - 2, COLS/2 - 2, 1, 1);
    scrollok(instrWindow, TRUE);

    regWindow = newwin(LINES/2 - 2, COLS/2 - 2, 1, 1);
    psrWindow = newwin(LINES/2 - 2, COLS/2 - 2, 1, 1);

    while(true) {
        wrefresh(instrWindow);
        wgetch(instrWindow);
    }

}


void Debugger::closeDisplay() {
    endwin();
}


void Debugger::displayInstr(std::string output) {
    waddstr(instrWindow, output.c_str());
}