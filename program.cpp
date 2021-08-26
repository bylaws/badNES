#include <fstream>
#include <iostream>

#include "program.h"

Program::Program(std::string path) {
	std::ifstream rom;
	uint8_t header[0x10];
	rom.open(path, std::ifstream::in | std::ifstream::binary);

	rom.read((char *)&header, sizeof(header));
	this->prg_rom_size = 16384 * header[4];
	this->chr_rom_size = 8192 * header[5];
	this->mirroring = !!(header[6] & 0b00000001);
	this->prg_ram = !!(header[6] & 0b0000010);
	this->trainer = !!(header[6] & 0b0000100);
	this->ignore_mirroring = !!(header[6] & 0b0001000);
	this->mapper = ((header[7] & 0b11110000) | (header[6] >> 4)) & 0xff;

	this->mirroring_id = this->mirroring | (this->ignore_mirroring << 1);
	std::cout << "PRG ROM size: " << this->prg_rom_size << std::endl;
	std::cout << "CHR ROM size: " << this->chr_rom_size << std::endl;
	std::cout << "Mirroring (0: horizontal, 1: vertical): " << this->mirroring << std::endl;
	std::cout << "PRG RAM (0: no PRG RAM, 1: has PRG RAM): " << this->prg_ram << std::endl;
	std::cout << "Trainer (0: no trainer, 1: has trainer): " << this->trainer << std::endl;
	std::cout << "Ignore mirroring (0: use mirroring, 1: use four screen VRAM): " << this->ignore_mirroring << std::endl;
	std::cout << "Mapper number: " << (unsigned int)this->mapper << std::endl;

	if (this->trainer)
		rom.seekg(0x200, std::ios_base::cur);
	
	this->prg = new uint8_t[this->prg_rom_size];
	if (this->chr_rom_size > 0)
		this->chr = new uint8_t[this->chr_rom_size];
	
	rom.read((char *)this->prg, this->prg_rom_size);
	if (this->chr_rom_size)
		rom.read((char *)this->chr, this->chr_rom_size);
	
	if (!this->chr_rom_size) {
		this->chr_rom_size = 0x2000;
		this->chr = new uint8_t[0x2000];
	}
}
