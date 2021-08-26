#pragma once

#include <stdint.h> 

class NES;

class Mapper {
	public:
		Mapper(NES *nes) {this->nes = nes; }


		virtual void write(uint16_t address, uint8_t value) {}
		virtual uint8_t read(uint16_t address) {return 0;}

		static Mapper *makeMapper(NES* nes, uint8_t mapper);
	protected:
		NES *nes;
};

