
class ARM7TDMI;
class Bus;


class Scheduler {
    
    public: 
        void loop();



    private:
        ARM7TDMI* cpu;
        Bus* bus;



};