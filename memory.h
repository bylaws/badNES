#pragma once

#include "mapper.h"

#include <stdint.h> 

class NES;

class Memory {
	public:
		Memory(NES *nes);

		void reset(void);

		void set_pressed_buttons(bool controller, int *buttons) {
			if (controller)
				this->buttons_p2 = buttons;
			else
				this->buttons_p1 = buttons;
		}

		Mapper *mapper(void) {
			return this->_mapper;
		}


		void stack_push(uint8_t &SP, uint8_t value);
		uint8_t stack_pull(uint8_t &SP);
		

		void write_address(uint16_t address, uint8_t value);
		uint8_t read_address(uint16_t address);
		uint16_t read_address16(uint16_t address);
		uint16_t read_address16_bug(uint16_t address);
	
		uint8_t *ram;
	private:
		NES *nes;
		
		Mapper *_mapper;
		
		int *buttons_p1;
		uint8_t index_p1;
		uint8_t strobe_p1;
		int *buttons_p2;
		uint8_t index_p2;
		uint8_t strobe_p2;
};
