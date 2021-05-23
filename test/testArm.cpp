#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <bitset>
#include <assert.h>

#include "../src/ARM7TDMI.h"
#include "../src/Bus.h"
#include "../src/GameBoyAdvance.h"



#define ASSERT_EQUAL(message, expected, actual) \
    if (expected != actual) { \
        DEBUG("ASSERTION FAILED: " << message <<  \
        " expected: " << expected << " != actual: " << actual << \
        "\n"); assert(false); } 

typedef struct cpu_log {
    uint32_t address;
    uint32_t instruction;
    uint32_t cpsr;
    uint32_t r[16];
    uint32_t cycles;
} cpu_log_t;

std::vector<cpu_log> getLogs(std::string path) {
    std::ifstream logFile;
    logFile.open(path);
    std::string line;

    std::vector<cpu_log> logs;
    std::string delimiter = ",";
    while (std::getline(logFile, line)) {
        // std::cout << line << std::endl;
        int currentPos = 0;
        int nextPos = (line.find(delimiter, currentPos) == std::string::npos)
                          ? line.size()
                          : line.find(delimiter, currentPos);
        std::string value;
        cpu_log log;

        value = line.substr(currentPos, nextPos - currentPos);
        log.address = std::stoul(value, nullptr, 16);
        currentPos = nextPos + 1;
        nextPos = (line.find(delimiter, currentPos) == std::string::npos)
                      ? line.size()
                      : line.find(delimiter, currentPos);

        value = line.substr(currentPos, nextPos - currentPos);
        log.instruction = std::stoul(value, nullptr, 16);
        currentPos = nextPos + 1;
        nextPos = (line.find(delimiter, currentPos) == std::string::npos)
                      ? line.size()
                      : line.find(delimiter, currentPos);

        value = line.substr(currentPos, nextPos - currentPos);
        log.cpsr = std::stoul(value, nullptr, 16);
        currentPos = nextPos + 1;
        nextPos = (line.find(delimiter, currentPos) == std::string::npos)
                      ? line.size()
                      : line.find(delimiter, currentPos);

        for (int i = 0; i < 16; i++) {
            value = line.substr(currentPos, nextPos - currentPos);
            log.r[i] = std::stoul(value, nullptr, 16);
            currentPos = nextPos + 1;
            nextPos = (line.find(delimiter, currentPos) == std::string::npos)
                          ? line.size()
                          : line.find(delimiter, currentPos);
        }

        value = line.substr(currentPos, nextPos - currentPos);
        log.cycles = std::stoul(value, nullptr, 10);
        currentPos = nextPos + 1;
        nextPos = (line.find(delimiter, currentPos) == std::string::npos)
                      ? line.size()
                      : line.find(delimiter, currentPos);

        logs.push_back(log);
    }
    return logs;
}


int main() {
    std::vector<cpu_log> logs = getLogs("arm.log");

    ARM7TDMI cpu;
    Bus bus;
    GameBoyAdvance gba(&cpu, &bus);
    gba.loadRom("arm.gba");

    for(cpu_log log : logs) {
        uint32_t instrAddress = cpu.getRegister(15);
        std::cout << "expectedAddr:\t" << log.address << std::endl;
        std::cout << "actualAddr:\t" << instrAddress << std::endl;
        ASSERT_EQUAL("address", (uint32_t)log.address, instrAddress)

        uint32_t cpsr = cpu.psrToInt(cpu.getCpsr());
        std::cout << "expectedCpsr:\t" << log.cpsr << std::endl;
        std::cout << "actualCpsr:\t" << cpsr << std::endl;
        ASSERT_EQUAL("cpsr", (uint32_t)log.cpsr, (uint32_t)cpsr)

        ASSERT_EQUAL("r0", log.r[0], cpu.getRegister(0))
        ASSERT_EQUAL("r1", log.r[1], cpu.getRegister(1))
        ASSERT_EQUAL("r2", log.r[2], cpu.getRegister(2))
        ASSERT_EQUAL("r3", log.r[3], cpu.getRegister(3))
        ASSERT_EQUAL("r4", log.r[4], cpu.getRegister(4))
        ASSERT_EQUAL("r5", log.r[5], cpu.getRegister(5))
        ASSERT_EQUAL("r6", log.r[6], cpu.getRegister(6))
        ASSERT_EQUAL("r7", log.r[7], cpu.getRegister(7))
        ASSERT_EQUAL("r8", log.r[8], cpu.getRegister(8))
        ASSERT_EQUAL("r9", log.r[9], cpu.getRegister(9))
        ASSERT_EQUAL("r10", log.r[10], cpu.getRegister(10))
        ASSERT_EQUAL("r11", log.r[11], cpu.getRegister(11))
        ASSERT_EQUAL("r12", log.r[12], cpu.getRegister(12))
        ASSERT_EQUAL("r13", log.r[13], cpu.getRegister(13))
        ASSERT_EQUAL("r14", log.r[14], cpu.getRegister(14))
        // add 4 to PC to account for pipelining
        ASSERT_EQUAL("r15", log.r[15], cpu.getRegister(15) + 4)
    
        cpu.step();

        std::cout << "expectedInstr:\t" << log.instruction << std::endl;
        std::cout << "actualInstr:\t" << cpu.getCurrentInstruction() << std::endl;
        std::cout << "actualInstr bits\t" << std::bitset<32>(cpu.getCurrentInstruction()).to_string() << std::endl;
        ASSERT_EQUAL("instruction", (uint32_t)log.instruction, (uint32_t)cpu.getCurrentInstruction())

        std::cout << std::endl;

    }
    return 0;
}
