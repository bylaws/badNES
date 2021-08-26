#include <iostream>

#include "mapper0.h"
#include "nes.h"

Mapper0::Mapper0(NES *nes) : Mapper(nes) {
	std::cout << "Mapper0 init" << std::endl;
	if (nes->program()->prg_rom_size <= 16384) {
		this->mirror_prg = true;
		std::cout << "Mirroring PRG ROM" << std::endl;
	} else {
		this->mirror_prg = false;
	}
}


uint8_t Mapper0::read(uint16_t address) {
	if (address < 0x2000)
		return this->nes->program()->chr[address];

	if (this->mirror_prg && address >= 0xc000)
		address -= 16384;

	address -= 0x8000;

	return this->nes->program()->prg[address];
}

void Mapper0::write(uint16_t address, uint8_t value) {
	if (address < 0x2000) {
		this->nes->program()->chr[address] = value;
		return;
	}
//Possible?
	if (this->mirror_prg && address >= 0xc000)
		address -= 16384;

	address -= 0x8000;

	this->nes->program()->prg[address] = value;
	return;
}

