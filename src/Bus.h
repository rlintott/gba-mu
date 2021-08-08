#pragma once

// define ndebug to suppress debug logs
// #define NDEBUG 0;

#include <array>
#include <vector>

#include "ARM7TDMI.h"


class Bus {
    // TODO: implement an OPEN BUS (ie if retreiving invalid mem location, return value last on bus)
    
    // TODO: NOTES: The GBA forcefully uses non-sequential timing at the beginning of each 128K-block of gamepak ROM, 
    // eg. "LDMIA [801fff8h],r0-r7" will have non-sequential timing at 8020000h.

   public:
    Bus();
    ~Bus();

    enum CycleType {
        SEQUENTIAL,
        NONSEQUENTIAL,
        INTERNAL
    };

    enum IORegister {
        IE = 0x04000200 - 0x04000000, // Interrupt Enable Register
        IF = 0x04000202 - 0x04000000, // Interrupt Request Flags / IRQ Acknowledge
        IME = 0x04000208 - 0x04000000, // Interrupt Master Enable Register
        DISPCNT = 0x04000000 - 0x04000000, // LCD control
        DISPSTAT = 0x04000004 - 0x04000000,  // General LCD Status (STAT,LYC)
        VCOUNT = 0x04000006 - 0x04000000, // Vertical Counter (LY)
        KEYINPUT = 0x04000130 - 0x04000000, // KEYINPUT - Key Status (R)
        KEYCNT = 0x04000132 - 0x04000000 // R/W  KEYCNT    Key Interrupt Control
    };

    /* General Internal Memory */

    // 00000000-00003FFF   BIOS - System ROM (16 KBytes) 16448
    std::vector<uint8_t> bios;
    // work ram! 02000000-0203FFFF (256kB) 263168
    std::vector<uint8_t> wRamBoard;
    // 03000000-03007FFF (32 kB) 32896
    std::vector<uint8_t> wRamChip;
    // 04000000-040003FE   I/O Registers 1028
    std::vector<uint8_t> iORegisters;

    /* Internal Display Memory */

    // 05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte) 1028
    std::vector<uint8_t> paletteRam;
    // 06000000-06017FFF   VRAM - Video RAM          (96 KBytes) 98688
    std::vector<uint8_t> vRam;
    // 07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte) 1028
    // TODO: VRAM and Palette RAM may be accessed during H-Blanking. 
    // OAM can accessed only if "H-Blank Interval Free" bit in DISPCNT register is set.
    std::vector<uint8_t> objAttributes;

    /* External Memory (Game Pak) */

    // 08000000-09FFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0
    // 0A000000-0BFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1
    // 0C000000-0DFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2
    std::vector<uint8_t> gamePakRom;
    // 0E000000-0E00FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width (65792)
    std::vector<uint8_t> gamePakSram;


    uint32_t read32(uint32_t address, CycleType accessType);
    uint16_t read16(uint32_t address, CycleType accessType);
    uint8_t read8(uint32_t address, CycleType accessType);

    void write32(uint32_t address, uint32_t word, CycleType accessType);
    void write16(uint32_t address, uint16_t halfWord, CycleType accessType);
    void write8(uint32_t address, uint8_t byte, CycleType accessType);

    void loadRom(std::string path);

    uint8_t getCurrentNWaitstate();
    uint8_t getCurrentSWaitstate();

    // resets the execution step timeline. 
    void reset();

    // adds eexecution step to timeline 
    // use if need to emualte extra exection steps besides the memory access
    void addCycleToExecutionTimeline(CycleType cycleType, uint32_t shift, uint8_t width);
    void printCurrentExecutionTimeline();

    void enterVBlank();
    void enterHBlank();
    void leaveVBlank();
    void leaveHBlank();

   private:
    uint8_t currentNWaitstate;
    uint8_t currentSWaitstate;

    uint32_t read(uint32_t address, uint8_t width, CycleType accessType);
    void write(uint32_t address, uint32_t value, uint8_t width, CycleType accessType);

    static constexpr uint32_t waitcntOffset = 0x204;

    uint8_t getWaitState0NCycles();
    uint8_t getWaitState1NCycles();
    uint8_t getWaitState2NCycles();

    uint8_t getWaitState0SCycles();
    uint8_t getWaitState1SCycles();
    uint8_t getWaitState2SCycles();

    uint8_t waitStateNVals[4] = {4,3,2,8};

    uint8_t waitState0SVals[2] = {2,1};
    uint8_t waitState1SVals[2] = {4,1};
    uint8_t waitState2SVals[2] = {8,1};

    std::array<std::string, 3> cycleTypesSerialized = {"S", "N", "I"};

    uint8_t executionTimelineSize = 0;
    std::array<uint8_t, 32> executionTimelineCycles;
    // TODO: questionable wither this should be owned by bus, could move into a scheduler class?
    std::array<CycleType, 32> executionTimelineCycleType;


};