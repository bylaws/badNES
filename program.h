#pragma once

#include <string>

class Program {
	public:
		Program(std::string path);

		uint8_t *chr;
		uint32_t chr_rom_size;
		uint8_t *prg;
		uint32_t prg_rom_size;

		uint8_t mapper;
		uint8_t mirroring_id;
		bool mirroring;
		bool ignore_mirroring;
		bool prg_ram;
		bool trainer;
};
