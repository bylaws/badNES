#include "memory.h"
#include "cpu.h"
#include "ppu.h"

class NES {
	public:
		NES();

		int cycle(void);
		
		void reset(void);
		
		Cpu6502 *cpu(void) {
			return this->cpu;
		}

		Ppu2c02 *ppu(void) {
			return this->ppu;
		}

		Memory *mem(void) {
			return this->mem;
		}
	private:
		Ppu2c02 *ppu;
		Cpu6502 *cpu;
		Memory *mem;
}
