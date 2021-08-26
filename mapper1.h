#pragma once

#include "mapper.h"

class Mapper1 : public Mapper {
	public:
		Mapper1(NES *nes);
		void write(uint16_t address, uint8_t value);
		uint8_t read(uint16_t address);
	private:
		uint8_t *sram;

		int prg_offset(int index);
		int chr_offset(int index);
		void update_control(uint8_t value);
		void update_offsets(void);

		int prg_offsets[2];
		int chr_offsets[2];
		uint8_t chr_mode;
		uint8_t prg_mode;

		uint8_t shift;
		uint8_t control;

		uint8_t prg_bank;
		
		uint8_t chr_bank0;
		uint8_t chr_bank1;
};
