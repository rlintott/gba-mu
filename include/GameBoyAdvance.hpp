#include <string>
#include <memory>

class GameBoyAdvanceImpl;

class GameBoyAdvance {
    public: 
        GameBoyAdvance();
        bool loadRom(std::string path);
        void setBreakpoint(uint32_t address);
        void enableDebugger();
        void runRom(); 
        void printCpuState();
        // TODO: more public methods   
    
    private: 
        std::shared_ptr<GameBoyAdvanceImpl> pimpl;

};