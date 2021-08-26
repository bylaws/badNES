#include <iostream>
#include "memory.h"
#include "ppu.h"
#include "nes.h"

Memory::Memory(NES *nes) {
	this->nes = nes;
	this->index_p1 = 0;
	this->ram = new uint8_t[0x10000];
	this->index_p2 = 0;
	this->strobe_p1 = 0;
	this->strobe_p2= 0;
}

void Memory::reset() {
	bool tmp = false;
	for (int i = 4; i < 0x2000; i += 4) {
		tmp = !tmp;
		if (i >= 0x100 && i < 0x200) {
			this->ram[i] = 0xFF;
			this->ram[i+1] = 0xFF;
			this->ram[i+2] = 0xFF;
			this->ram[i+3] = 0xFF;
		} else {
			this->ram[i] = tmp ? 255 : 0;
			this->ram[i+1] = tmp ? 255 : 0;
			this->ram[i+2] = tmp ? 255 : 0;
			this->ram[i+3] = tmp ? 255 : 0;
		}
	}
	this->_mapper = Mapper::makeMapper(this->nes, this->nes->program()->mapper);
}



void Memory::stack_push(uint8_t &SP, uint8_t value) {
	this->ram[0x100 + SP] = value;

	if (SP == 0)
		SP = 0xff;
	else
		SP--;
}

uint8_t Memory::stack_pull(uint8_t &SP) {
	if (SP == 0xff)
		SP = 0;
	else
		SP++;

	return this->ram[0x100 + SP];
}

void Memory::write_address(uint16_t address, uint8_t value) {
	if (address < 0x2000) {
		this->ram[address] = value;
	} else if (address < 0x4000) {
		this->nes->ppu()->write(0x2000 + address % 8, value);
	} else if (address == 0x4014) {
		this->nes->ppu()->write(address, value);
	} else if (address == 0x4016) {
		this->strobe_p1 = value;
		this->strobe_p2 = value;
		if (value & 0b1) {
			this->index_p1 = 0;
			this->index_p2 = 0;
		}
		return;
	} else if (address >= 0x6000) {
		this->mapper()->write(address, value);
	}
}

uint16_t Memory::read_address16(uint16_t address) {
	return (uint16_t)read_address(address) | ((uint16_t)read_address(address + 1) << 8);
}

uint16_t Memory::read_address16_bug(uint16_t address) {
	return ((uint16_t)read_address((address & 0xFF00) | ((address + 1) & 0xFF)) << 8) | (uint16_t)read_address(address);
}

uint8_t Memory::read_address(uint16_t address) {
	if (address < 0x2000) {
		return this->ram[address];
	} else if (address < 0x4000) {
		return this->nes->ppu()->read(0x2000 + address % 8);
	} else if (address == 0x4014) {
		return this->nes->ppu()->read(address);
	} else if (address == 0x4016) {
		uint8_t ret = 0; 

		if (this->index_p1 < 8 && this->buttons_p1[this->index_p1])
			ret = 1;

		this->index_p1++;

		if (this->strobe_p1 & 1)
			this->index_p1 = 0;

		return ret;
	} else if (address == 0x4017) {
		uint8_t ret = 0; 

		if (this->index_p2 < 8 && this->buttons_p2[this->index_p2])
			ret = 1;

		this->index_p2++;

		if (this->strobe_p2 & 1)
			this->index_p2 = 0;

		return ret;
	} else if (address >= 0x6000) {
		return this->mapper()->read(address);
	}
	return 0xDE;
}
