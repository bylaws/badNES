#include <iostream>

#include "mapper1.h"
#include "nes.h"

Mapper1::Mapper1(NES *nes) : Mapper(nes) {
	std::cout << "Mapper1 init" << std::endl;
	this->prg_mode = 0;
	this->chr_mode = 0;
	this->control = 0;
	this->prg_bank = 0;
	this->chr_bank0 = 0;
	this->chr_bank1 = 0;
	this->shift = 0x10;
	this->prg_offsets[1] = prg_offset(-1);
	this->sram = new uint8_t[0x2000];
}

int Mapper1::prg_offset(int index) {
	if (index >= 0x80)
		index -= 0x100;

	index %= this->nes->program()->prg_rom_size / 0x4000;
	int offset = index * 0x4000;

	if (offset < 0)
		offset += this->nes->program()->prg_rom_size;

	return offset;
}

int Mapper1::chr_offset(int index) {
	if (index >= 0x80)
		index -= 0x100;

	index %= this->nes->program()->chr_rom_size / 0x1000;
	int offset = index * 0x1000;

	if (offset < 0)
		offset += this->nes->program()->chr_rom_size;

	return offset;
}

void Mapper1::update_offsets() {
	if (this->prg_mode == 0 || this->prg_mode == 1) {
		this->prg_offsets[0] = prg_offset((int)(this->prg_bank & 0xfe));
		this->prg_offsets[1] = prg_offset((int)(this->prg_bank | 1));
	} else if (this->prg_mode == 2) {
		this->prg_offsets[0] = 0;
		this->prg_offsets[1] = prg_offset((int)(this->prg_bank));
	} else {
		this->prg_offsets[0] = prg_offset((int)(this->prg_bank));
		this->prg_offsets[1] = prg_offset(-1);
	}

	if (this->chr_mode == 0) {
		this->chr_offsets[0] = chr_offset((int)(this->chr_bank0 & 0xfe));
		this->chr_offsets[1] = chr_offset((int)(this->chr_bank0 | 1));
	} else {
		this->chr_offsets[0] = chr_offset((int)(this->chr_bank0));
		this->chr_offsets[1] = chr_offset((int)(this->chr_bank1));
	}
}

void Mapper1::update_control(uint8_t value) {
	this->control = value;
	this->chr_mode = (value >> 4) & 1;
	this->prg_mode = (value >> 2) & 3;
	this->nes->ppu()->set_mirroring(value & 3);
	update_offsets();
}

uint8_t Mapper1::read(uint16_t address) {
	uint16_t bank;
	uint16_t offset;
	if (address < 0x2000) {
		bank = address / 0x1000;
		offset = address % 0x1000;
		return this->nes->program()->chr[chr_offsets[bank] + (int)offset];
	} else if (address >= 0x8000) {
		address -= 0x8000;
		bank = address / 0x4000;
		offset = address % 0x4000;
		return this->nes->program()->prg[prg_offsets[bank] + (int)offset];
	} else { //SRAM
		return this->sram[address - 0x6000];
	}
}

void Mapper1::write(uint16_t address, uint8_t value) {
	uint16_t bank;
	uint16_t offset;
	if (address < 0x2000) {
		bank = address / 0x1000;
		offset = address % 0x1000;
		this->nes->program()->chr[chr_offsets[bank] + (int)offset] = value;
		return;
	} else if (address >= 0x8000) {
		if ((uint8_t)(value & (uint8_t)0x80)) {
			this->shift = 0x10;
			update_control(this->control | 0x0c);
		} else {
			bool complete = this->shift & 1;
			this->shift >>= 1;
			this->shift |= (value & 1) << 4;
			if (complete) {
				if (address <= 0x9fff) {
					update_control(this->shift);
				} else if (address <= 0xbfff) {
					this->chr_bank0 = this->shift;
					update_offsets();
				} else if (address <= 0xdfff) {
					this->chr_bank1 = this->shift;
					update_offsets();
				} else {
					this->prg_bank = this->shift & 0xf;
					update_offsets();
				}

				this->shift = 0x10;
			}
		}
		return;
	} else { //SRAM
		this->sram[address - 0x6000] = value;
		return;
	}
}

