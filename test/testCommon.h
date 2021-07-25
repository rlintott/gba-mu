#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <bitset>
#include <assert.h>



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
};

