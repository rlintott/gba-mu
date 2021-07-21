
class ARM7TDMI;
class Bus;


class Scheduler {
    
    public: 
        void runLoop();



    private:
        ARM7TDMI* cpu;
        Bus* bus;



};