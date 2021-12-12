

#include "../ARM7TDMI.h"
#include "../Bus.h"

template<uint16_t op>
ARM7TDMI::FetchPCMemoryAccess ARM7TDMI::armPsrHandler(uint32_t instruction, ARM7TDMI* cpu) {
    assert(!(instruction & 0x0C000000));
    //assert(!sFlagSet(instruction));
    DEBUG("in psr handler\n");
    DEBUG("psr instruction: " << std::bitset<32>(instruction).to_string() << std::endl);
    // bit 25: I - Immediate Operand Flag
    // (0=Register, 1=Immediate) (Zero for MRS)
    constexpr bool immediate = (op & 0x200);
    // bit 22: Psr - Source/Destination PSR
    // (0=CPSR, 1=SPSR_<current mode>)
    constexpr bool psrSource = (op & 0x040);
    constexpr bool opcode = (op & 0x020);

    //DEBUGWARN(immediate << " <- immediate?\n");
    //DEBUGWARN(psrSource << " <- psrSource?\n");

    // bit 21: special opcode for PSR
    if constexpr(opcode) {
        DEBUG("MSR{cond}\n");
        assert((instruction & 0x0000F000) == 0x0000F000);
        uint8_t fscx = (instruction & 0x000F0000) >> 16;
        ARM7TDMI::ProgramStatusRegister *psr = (!psrSource ? &(cpu->cpsr) : cpu->getCurrentModeSpsr());
        if constexpr (immediate) {
            //DEBUGWARN("MSR{cond} immediate\n");
            uint32_t immValue = (uint32_t)(instruction & 0x000000FF);
            uint8_t shift = (instruction & 0x00000F00) >> 7;
            cpu->transferToPsr(ARM7TDMI::aluShiftRor(immValue, shift), fscx, psr);
        } else {  // register
            DEBUG("MSR{cond} reg\n");
            //DEcpu->BUGWARN("rm: " << (uint32_t)getRm(instruction) << "\n");
            assert(!(instruction & 0x00000FF0));
            assert(ARM7TDMI::getRm(instruction) != ARM7TDMI::PC_REGISTER);
            // TODO: refactor this, don't have to pass in a pointer to the psr
            cpu->transferToPsr(cpu->getRegister(ARM7TDMI::getRm(instruction)), fscx, psr);
        }
    } else {
        DEBUG("MRS{cond}\n");
        assert(!immediate);
        assert(ARM7TDMI::getRn(instruction) == 0xF);
        assert(!(instruction & 0x00000FFF));
        uint8_t rd = ARM7TDMI::getRd(instruction);
        DEBUG((uint32_t)rd << " <- rd\n");
        if constexpr (!psrSource) {
            // TODO: this psrToInt and psr stuff maybe can be optimized, it's messy
            cpu->setRegister(rd, ARM7TDMI::psrToInt(cpu->cpsr));
        } else {
            cpu->setRegister(rd, ARM7TDMI::psrToInt(*(cpu->getCurrentModeSpsr())));
        }       
    }

    return ARM7TDMI::FetchPCMemoryAccess::SEQUENTIAL;
}
