
#include <cstdint>
#include <array>
#include <string>

class ARM7TDMI;
class Bus;

class Debugger {

    

    public: 
        void updateDebuggerState(ARM7TDMI* cpu, Bus* bus);
        void printState();
        void getInput();
        void step();
        bool stepMode = false;

    private:
        uint32_t currInstrAddress;
        std::string currInstr;
        
        uint32_t breakpointAddress;
        std::array<uint32_t, 7> watchAddresses;
        std::array<uint8_t, 7> watchAddressWidths;

};