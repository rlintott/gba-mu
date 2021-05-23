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
        cpu.step();
        std::cout << "expectedInstr:\t" << log.instruction << std::endl;
        std::cout << "actualInstr:\t" << cpu.getCurrentInstruction() << std::endl;
        std::cout << "actualInstr bits\t" << std::bitset<32>(cpu.getCurrentInstruction()).to_string() << std::endl;
        ASSERT_EQUAL("instruction", (uint32_t)log.instruction, (uint32_t)cpu.getCurrentInstruction())
    }
    return 0;
}
