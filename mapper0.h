#pragma once

#include "mapper.h"

class Mapper0 : public Mapper {
	public:
		Mapper0(NES *nes);
		void write(uint16_t address, uint8_t value);
		uint8_t read(uint16_t address);
	private:
		bool mirror_prg;
};
