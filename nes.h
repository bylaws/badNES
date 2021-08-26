#pragma once
#include <string>
#include "memory.h"
#include "cpu.h"
#include "ppu.h"
#include "program.h"

class NES {
	public:
		NES();

		void load(std::string path);
		void cycle(void);
		
		void reset(void);
		
		//                 x    y    color
		void (*draw_pixel)(int, int, uint8_t);
		void (*frame_done)();

		void set_render(void (*draw_pixel)(int, int, uint8_t), void (*frame_done)());

		Cpu6502 *cpu(void) {
			return this->_cpu;
		}

		Ppu2c02 *ppu(void) {
			return this->_ppu;
		}

		Memory *mem(void) {
			return this->_mem;
		}

		Program *program(void) {
			return this->_program;
		}
		
		uint64_t cycles;
	private:
		Ppu2c02 *_ppu;
		Cpu6502 *_cpu;
		Memory *_mem;
		Program *_program;
};
