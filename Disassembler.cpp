
#include "Disassembler.h"
#include <bitset>



std::unordered_map<ARM7TDMI::AluOpcode, std::string> Disassembler::opcodeToName = {
    {ARM7TDMI::AND, "AND"},
    {ARM7TDMI::EOR, "EOR"},
    {ARM7TDMI::SUB, "SUB"},
    {ARM7TDMI::RSB, "RSB"},
    {ARM7TDMI::ADD, "ADD"},
    {ARM7TDMI::ADC, "ADC"},
    {ARM7TDMI::SBC, "SBC"},
    {ARM7TDMI::RSC, "RSC"},
    {ARM7TDMI::TST, "TST"},
    {ARM7TDMI::TEQ, "TEQ"},
    {ARM7TDMI::CMP, "CMP"},
    {ARM7TDMI::CMN, "CMN"},
    {ARM7TDMI::ORR, "ORR"},
    {ARM7TDMI::MOV, "MOV"},
    {ARM7TDMI::BIC, "BIC"},
    {ARM7TDMI::MVN, "MVN"}
};

void Disassembler::disassembleDataProcessing(uint32_t instruction) {
    arminstr_t armInstr;
    armInstr.raw = instruction;
    uint8_t opcode = armInstr.parsed.DATA_PROCESSING.opcode;
    data_processing_t dataProc = armInstr.parsed.DATA_PROCESSING;

    std::cout << opcodeToName[ARM7TDMI::AluOpcode(opcode)] << 
        (dataProc.s ? "S" : "") << "\t" << 
        "rd=" << dataProc.rd << " " <<
        "rn=" << dataProc.rn << " " <<
        "operand2=" << std::bitset<12>(dataProc.operand2).to_string();
    
    std::cout << std::endl;
}

void Disassembler::disassembleMultiply(uint32_t instruction) {

}

void Disassembler::disassembleMultiplyLong(uint32_t instruction) {

}

void Disassembler::disassembleSingleDataSwap(uint32_t instruction) {

}
