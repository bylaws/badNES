#pragma once
#include "nes.h"

class Mappers {
	public:
		MappersgTg(NES *nes);

		void reset();

//		void write(uint16_t address, uint8_t value);
		void (*write)(uint16_t, uint8_t);
		uint8_t (*read)(uint16_t);
//		uint8_t read(uint16_t address);

	private:
		uint8_t *sram;
		NES *nes;
};

