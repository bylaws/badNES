#include "nes.h"
#include "memory.h"
#include "program.h"

NES::NES() {
	this->_mem = new Memory(this);
	this->_cpu = new Cpu6502(this);
	this->_ppu = new Ppu2c02(this);
	cycles = 0;
}

void NES::load(std::string path) {
	this->_program = new Program(path);
}
void NES::reset() {
	this->_mem->reset();
	this->_cpu->reset();
}

void NES::set_render(void (*draw_pixel)(int, int, uint8_t), void (*frame_done)()) {
	this->draw_pixel = draw_pixel;
	this->frame_done = frame_done;
}

void NES::cycle() {
	int count = this->_cpu->step_one();
	if (count == 0) {
		count = 1;
	} else {
		cycles += count;
	}
	for (count *= 3; count > 0; count--)
		this->_ppu->run_cycle();
}
