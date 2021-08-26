#include <iostream>
#include "cpu.h"
#include "nes.h"

Cpu6502::Cpu6502(NES *nes) {
	this->nes = nes;
}

void Cpu6502::reset() {
	this->A = 0;
	this->X = 0;
	this->Y = 0;
	this->penalty = 0;
	this->nmi_pending = false;
	this->needs_crossed_page_cycles = false;
	this->S = STATUS_IRQ_DISABLE | STATUS_RESERVED;
	this->SP = 0xfd;
	this->PC = this->nes->mem()->read_address16(0xfffc);
	this->current_instruction = this->PC;
}

uint16_t Cpu6502::get_instruction_data(AddressingMode mode) {
	if (mode == AddressingMode::IMMEDIATE)
		return this->current_instruction + 1;
	else if (mode == AddressingMode::ACCUMULATOR)
		return this->A;
	else if (mode == AddressingMode::ABSOLUTE)
		return this->nes->mem()->read_address16(this->current_instruction + 1);
	else if (mode == AddressingMode::ZERO_PAGE)
		return this->nes->mem()->read_address(this->current_instruction + 1);
	else if (mode == AddressingMode::RELATIVE) {
		uint16_t address = this->nes->mem()->read_address(this->current_instruction + 1);
		if (address < 0x80)
			address += 2 + this->current_instruction;
		else
			address +=  2+ this->current_instruction - 256;
		return address;
	} else if (mode == AddressingMode::INDEXED_ZERO_PAGE_X)
		return (this->nes->mem()->read_address(this->current_instruction + 1) + this->X) & 0xff;
	else if (mode == AddressingMode::INDEXED_ZERO_PAGE_Y)
		return (this->nes->mem()->read_address(this->current_instruction + 1) + this->Y) & 0xff;
	else if (mode == AddressingMode::INDEXED_ABSOLUTE_X) {
		uint16_t address_orig = this->nes->mem()->read_address16(this->current_instruction + 1);
		uint16_t address = address_orig + (uint16_t)this->X;
		this->needs_crossed_page_cycles = page_crossed(address_orig, address);
		return address;
	}else if (mode == AddressingMode::INDEXED_ABSOLUTE_Y) {
		uint16_t address_orig = this->nes->mem()->read_address16(this->current_instruction + 1);
		uint16_t address = address_orig + (uint16_t)this->Y;
		this->needs_crossed_page_cycles = page_crossed(address_orig, address);
		return address;
	}else if (mode == AddressingMode::INDEXED_INDIRECT) {
		uint16_t address = this->nes->mem()->read_address(this->current_instruction + 1);

		address = (address + this->X) &  0xff;
		address = this->nes->mem()->read_address16_bug(address);
		return address;
	} else if (mode == AddressingMode::INDIRECT_INDEXED) {
		uint16_t address = this->nes->mem()->read_address(this->current_instruction + 1);
		address = this->nes->mem()->read_address16_bug(address);
		
		address = (address + this->Y);
		this->needs_crossed_page_cycles = page_crossed(address-(uint16_t)this->Y, address);
		return address;
	} else //if (mode == AddressingMode::INDIRECT)
		return this->nes->mem()->read_address16_bug(this->nes->mem()->read_address16(this->current_instruction + 1));
}

int Cpu6502::get_size(AddressingMode mode) {
	if (mode == AddressingMode::IMPLIED)
		return 1;
	else if (mode == AddressingMode::ACCUMULATOR)
		return 1;
	else if (mode == AddressingMode::IMMEDIATE)
		return 2;
	else if (mode == AddressingMode::ABSOLUTE)
		return 3;
	else if (mode == AddressingMode::ZERO_PAGE)
		return 2;
	else if (mode == AddressingMode::RELATIVE)
		return 2;
	else if (mode == AddressingMode::INDEXED_ZERO_PAGE_X)
		return 2;
	else if (mode == AddressingMode::INDEXED_ZERO_PAGE_Y)
		return 2;
	else if (mode == AddressingMode::INDEXED_ABSOLUTE_X)
		return 3;
	else if (mode == AddressingMode::INDEXED_ABSOLUTE_Y)
		return 3;
	else if (mode == AddressingMode::INDEXED_INDIRECT)
		return 2;
	else if (mode == AddressingMode::INDIRECT_INDEXED)
		return 2;
	else// if (mode == AddressingMode::INDIRECT)
		return 3;
}

void Cpu6502::set_flags(uint8_t byte) {
	if (byte & 0x80)
		this->S = this->S | STATUS_NEGATIVE;
	else
		this->S = this->S & ~STATUS_NEGATIVE;

	if (byte == 0)
		this->S = this->S | STATUS_ZERO;
	else
		this->S = this->S & ~STATUS_ZERO;
}

bool Cpu6502::page_crossed(uint16_t a, uint16_t b) {
	return a&0xFF00 != b&0xFF00;
}

int Cpu6502::instruction_ADC(AddressingMode mode) {
	uint8_t data2 = this->nes->mem()->read_address(get_instruction_data(mode));

	if (this->do_sbc)
		data2 = ~data2;

	unsigned int data =  this->A + data2 + (this->S & STATUS_CARRY);

	if (data > 0xff)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;

	data &= 0xff;

	if (~(this->A ^ data2) & ((this->A ^ data) & 0x80))
		this->S |= STATUS_OVERFLOW;
	else
		this->S &= ~STATUS_OVERFLOW;


	this->A = (uint8_t)data;
	set_flags(this->A);

	return 0;
}

int Cpu6502::instruction_AND(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	this->A &= data;
	set_flags(this->A & 0xff);

	return 0;
}

int Cpu6502::instruction_ASL(AddressingMode mode) {
	uint16_t address;
	uint8_t data;

	if (mode == AddressingMode::ACCUMULATOR)
		data = this->A;
	else {
		address = get_instruction_data(mode);
		data = this->nes->mem()->read_address(address);
	}

	if (data & 0b10000000)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;

	data *= 2; // left shift
	data &= 0b11111111;
	set_flags(data);

	if (mode == AddressingMode::ACCUMULATOR)
		this->A = data;
	else
		this->nes->mem()->write_address(address, data);

	return 0;
}

int Cpu6502::instruction_BCC(AddressingMode mode) {
	if (!(this->S & STATUS_CARRY)) {
		uint16_t old_PC = this->PC;
		this->PC = get_instruction_data(mode);
		return 1 + page_crossed(old_PC, this->PC); // Branch succeeded
	}

	return 0;
}

int Cpu6502::instruction_BCS(AddressingMode mode) {
	if (this->S & STATUS_CARRY) {
		uint16_t old_PC = this->PC;
		this->PC = get_instruction_data(mode);
		return 1 + page_crossed(old_PC, this->PC); // Branch succeeded
	}

	return 0;
}

int Cpu6502::instruction_BEQ(AddressingMode mode) {
	if (this->S & STATUS_ZERO) {
		uint16_t old_PC = this->PC;
		this->PC = get_instruction_data(mode);
		return 1 + page_crossed(old_PC, this->PC); // Branch succeeded
	}

	return 0;
}

int Cpu6502::instruction_BIT(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));

	if ((this->A & data) == 0)
		this->S |= STATUS_ZERO;
	else
		this->S &= ~STATUS_ZERO;

	if (data & 0b01000000)
		this->S |= STATUS_OVERFLOW;
	else
		this->S &= ~STATUS_OVERFLOW;

	if (data & 0b10000000)
		this->S |= STATUS_NEGATIVE;
	else
		this->S &= ~STATUS_NEGATIVE;

	return 0;
}

int Cpu6502::instruction_BMI(AddressingMode mode) {
	if (this->S & STATUS_NEGATIVE) {
		uint16_t old_PC = this->PC;
		this->PC = get_instruction_data(mode);
		return 1 + page_crossed(old_PC, this->PC); // Branch succeeded
	}
	
	return 0;
}

int Cpu6502::instruction_BNE(AddressingMode mode) {
	if (!(this->S & STATUS_ZERO)) {
		uint16_t old_PC = this->PC;
		this->PC = get_instruction_data(mode);
		return 1 + page_crossed(old_PC, this->PC); // Branch succeeded
	}
	
	return 0;
}

int Cpu6502::instruction_BPL(AddressingMode mode) {
	if (!(this->S & STATUS_NEGATIVE)) {
		uint16_t old_PC = this->PC;
		this->PC = get_instruction_data(mode);
		return 1 + page_crossed(old_PC, this->PC); // Branch succeeded
	}
	
	return 0;
}

int Cpu6502::instruction_BRK(AddressingMode mode) {
	this->nes->mem()->stack_push(this->SP, ((this->PC)& 0xff00) >> 8);
	this->nes->mem()->stack_push(this->SP, (this->PC)& 0xff);
	if (this->nmi_pending)
		this->nes->mem()->stack_push(this->SP, 0);
	else
		this->nes->mem()->stack_push(this->SP, this->S);
 
	if (this->nmi_pending)
		this->PC = this->nes->mem()->read_address16(0xfffa);
	else
		this->PC = this->nes->mem()->read_address16(0xfffe);

	if (this->nmi_pending)
		this->S |= STATUS_IRQ_DISABLE;
	return 0;
}

int Cpu6502::instruction_BVC(AddressingMode mode) {
	if (!(this->S & STATUS_OVERFLOW)) {
		this->PC = get_instruction_data(mode);
		return 1; // Branch succeeded
	}
	
	return 0;
}

int Cpu6502::instruction_BVS(AddressingMode mode) {
	if (this->S & STATUS_OVERFLOW) {
		this->PC = get_instruction_data(mode);
		return 1; // Branch succeeded
	}
	
	return 0;
}

int Cpu6502::instruction_CLC(AddressingMode mode) {
	this->S &= ~STATUS_CARRY;
	
	return 0;
}

int Cpu6502::instruction_CLD(AddressingMode mode) {
	this->S &= ~STATUS_DECIMAL;
	
	return 0;
}

int Cpu6502::instruction_CLI(AddressingMode mode) {
	this->S &= ~STATUS_IRQ_DISABLE;
	
	return 0;
}

int Cpu6502::instruction_CLV(AddressingMode mode) {
	this->S &= ~STATUS_OVERFLOW;
	
	return 0;
}

int Cpu6502::instruction_CMP(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	if (this->A >= data)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;
	
	data = ((this->A) - (data));
	set_flags(data);

	return 0;
}

int Cpu6502::instruction_CPX(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	if (this->X >= data)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;
	
	data = ((this->X) - (data));
	set_flags(data & 0xff);

	return 0;
}

int Cpu6502::instruction_CPY(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	if (this->Y >= data)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;
	
	data = ((this->Y) - (data));
	set_flags(data & 0xff);

	return 0;
}

int Cpu6502::instruction_DEC(AddressingMode mode) {
	uint16_t address = get_instruction_data(mode);
	uint8_t data = this->nes->mem()->read_address(address);
	
	data = (data - 1) & 0xff;
	set_flags(data);

	this->nes->mem()->write_address(address, data);
	return 0;
}

int Cpu6502::instruction_DEX(AddressingMode mode) {
	this->X = (this->X - 1) & 0xff;
	set_flags(this->X);
	return 0;
}

int Cpu6502::instruction_DEY(AddressingMode mode) {
	this->Y = (this->Y - 1) & 0xff;
	set_flags(this->Y);
	return 0;
}

int Cpu6502::instruction_EOR(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));

	this->A ^= data & 0xff;
	set_flags(this->A);
	return 0;
}

int Cpu6502::instruction_INC(AddressingMode mode) {
	uint16_t address = get_instruction_data(mode);
	uint8_t data = this->nes->mem()->read_address(address);
	
	data = (data + 1) & 0xff;
	set_flags(data);

	this->nes->mem()->write_address(address, data);
	return 0;
}

int Cpu6502::instruction_INX(AddressingMode mode) {
	this->X = (this->X + 1) & 0xff;
	set_flags(this->X);
	return 0;
}

int Cpu6502::instruction_INY(AddressingMode mode) {
	this->Y = (this->Y + 1) & 0xff;
	set_flags(this->Y);
	return 0;
}

int Cpu6502::instruction_JMP(AddressingMode mode) {
	this->PC = get_instruction_data(mode);
	return 0;
}

int Cpu6502::instruction_JSR(AddressingMode mode) {
	this->nes->mem()->stack_push(this->SP, ((this->PC - 1)& 0xff00) >> 8);
	this->nes->mem()->stack_push(this->SP, (this->PC - 1)& 0xff);
	this->PC = get_instruction_data(mode);
	return 0;
}

int Cpu6502::instruction_LDA(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	this->A = data;
	set_flags(data);

	return 0;
}

int Cpu6502::instruction_LDX(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	this->X = data;
	set_flags(data);

	return 0;
}

int Cpu6502::instruction_LDY(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	this->Y = data;
	set_flags(data);

	return 0;
}

int Cpu6502::instruction_LSR(AddressingMode mode) {
	uint8_t data;
	uint16_t address;

	if (mode == AddressingMode::ACCUMULATOR)
		data = this->A;
	else {
		address = get_instruction_data(mode);
		data = this->nes->mem()->read_address(address);
	}

	if (data & 1)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;

	data = (data >> 1) & 0xff; //Right shift
	set_flags(data);

	if (mode == AddressingMode::ACCUMULATOR)
		this->A = data;
	else
		this->nes->mem()->write_address(address, data);

	return 0;
}

int Cpu6502::instruction_NOP(AddressingMode mode) {
	return 0;
}

int Cpu6502::instruction_ORA(AddressingMode mode) {
	uint8_t data = this->nes->mem()->read_address(get_instruction_data(mode));
	
	this->A = (uint8_t)((uint8_t)this->A | (uint8_t)data);
	set_flags(this->A);

	return 0;
}

int Cpu6502::instruction_PHA(AddressingMode mode) {
	this->nes->mem()->stack_push(this->SP, this->A);

	return 0;
}

int Cpu6502::instruction_PHP(AddressingMode mode) {
	this->nes->mem()->stack_push(this->SP, this->S | STATUS_BREAK_MODE);

	return 0;
}

int Cpu6502::instruction_PLA(AddressingMode mode) {
	this->A = this->nes->mem()->stack_pull(this->SP);

	set_flags(this->A);

	return 0;
}

int Cpu6502::instruction_PLP(AddressingMode mode) {
	uint8_t status = this->nes->mem()->stack_pull(this->SP);

	status &= ~STATUS_BREAK_MODE;
	status &= ~STATUS_RESERVED;

	status |= this->S & STATUS_BREAK_MODE;
	status |= this->S & STATUS_RESERVED;

	this->S = status;

	return 0;
}

int Cpu6502::instruction_ROL(AddressingMode mode) {
	uint8_t data;
	uint16_t address;

	if (mode == AddressingMode::ACCUMULATOR)
		data = this->A;
	else {
		address = get_instruction_data(mode);
		data = this->nes->mem()->read_address(address);
	}
	
	uint8_t old_status = this->S;

	if (data & 0b10000000)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;

	data = (data << 1) & 0xff; //Left shift
	data |= old_status & 0b00000001; //Set data bit 0 old carry flag
	set_flags(data);

	if (mode == AddressingMode::ACCUMULATOR)
		this->A = data;
	else
		this->nes->mem()->write_address(address, data);

	return 0;
}

int Cpu6502::instruction_ROR(AddressingMode mode) {
	uint8_t data;
	uint16_t address;

	if (mode == AddressingMode::ACCUMULATOR)
		data = this->A;
	else {
		address = get_instruction_data(mode);
		data = this->nes->mem()->read_address(address);
	}
	
	uint8_t old_status = this->S;

	if (data & 0b00000001)
		this->S |= STATUS_CARRY;
	else
		this->S &= ~STATUS_CARRY;

	data = data >> 1; //Right shift
	data |= (old_status & 0b00000001) <<  7; //Set data bit 7 old carry flag
	set_flags(data);

	if (mode == AddressingMode::ACCUMULATOR)
		this->A = data;
	else
		this->nes->mem()->write_address(address, data);

	return 0;
}

int Cpu6502::instruction_RTI(AddressingMode mode) {
	int cycles = instruction_PLP(mode) + instruction_RTS(mode);
	this->PC -= 1;
	return cycles;
}

int Cpu6502::instruction_RTS(AddressingMode mode) {
	uint16_t tmp = this->nes->mem()->stack_pull(this->SP);
	uint16_t tmp2 = this->nes->mem()->stack_pull(this->SP);
	this->PC = (uint16_t)tmp2 << 8 & 0b1111111100000000;
	this->PC |= tmp & 0b0000000011111111;
	this->PC = (this->PC + 1);
	return 0;
}

int Cpu6502::instruction_SBC(AddressingMode mode) {
	this->do_sbc = true;
	int ret = instruction_ADC(mode);
	this->do_sbc = false;
	return ret;
}

int Cpu6502::instruction_SEC(AddressingMode mode) {
	this->S |= STATUS_CARRY;
	
	return 0;
}

int Cpu6502::instruction_SED(AddressingMode mode) {
	this->S |= STATUS_DECIMAL;
	
	return 0;
}

int Cpu6502::instruction_SEI(AddressingMode mode) {
	this->S |= STATUS_IRQ_DISABLE;
	
	return 0;
}

int Cpu6502::instruction_STA(AddressingMode mode) {
	uint16_t address = get_instruction_data(mode);
	
	this->nes->mem()->write_address(address, this->A);

	return 0;
}

int Cpu6502::instruction_STX(AddressingMode mode) {
	uint16_t address = get_instruction_data(mode);
	
	this->nes->mem()->write_address(address, this->X);

	return 0;
}

int Cpu6502::instruction_STY(AddressingMode mode) {
	uint16_t address = get_instruction_data(mode);
	
	this->nes->mem()->write_address(address, this->Y);

	return 0;
}

int Cpu6502::instruction_TAX(AddressingMode mode) {
	this->X = this->A;
	set_flags(this->X);
	
	return 0;
}

int Cpu6502::instruction_TAY(AddressingMode mode) {
	this->Y = this->A;
	set_flags(this->Y);
	
	return 0;
}

int Cpu6502::instruction_TSX(AddressingMode mode) {
	this->X = this->SP;
	set_flags(this->X);
	
	return 0;
}

int Cpu6502::instruction_TXA(AddressingMode mode) {
	this->A = this->X;
	set_flags(this->A);
	
	return 0;
}

int Cpu6502::instruction_TXS(AddressingMode mode) {
	this->SP = this->X;
	
	return 0;
}

int Cpu6502::instruction_TYA(AddressingMode mode) {
	this->A = this->Y;
	set_flags(this->A);
	
	return 0;
}

const Instruction instruction_map[] = {
		{0x00, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x01, AddressingMode::INDEXED_INDIRECT, 6, 0, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x02, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x03, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x04, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x05, AddressingMode::ZERO_PAGE, 3, 0, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x06, AddressingMode::ZERO_PAGE, 5, 0, "instruction_ASL", &Cpu6502::instruction_ASL},
		{0x07, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x08, AddressingMode::IMPLIED, 3, 0, "instruction_PHP", &Cpu6502::instruction_PHP},
		{0x09, AddressingMode::IMMEDIATE, 2, 0, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x0a, AddressingMode::ACCUMULATOR, 2, 0, "instruction_ASL", &Cpu6502::instruction_ASL},
		{0x0b, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x0c, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x0d, AddressingMode::ABSOLUTE, 4, 0, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x0e, AddressingMode::ABSOLUTE, 6, 0, "instruction_ASL", &Cpu6502::instruction_ASL},
		{0x0f, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x10, AddressingMode::RELATIVE, 2, 1, "instruction_BPL", &Cpu6502::instruction_BPL},
		{0x11, AddressingMode::INDIRECT_INDEXED, 5, 1, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x12, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x13, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x14, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x15, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x16, AddressingMode::INDEXED_ZERO_PAGE_X, 6, 0, "instruction_ASL", &Cpu6502::instruction_ASL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x18, AddressingMode::IMPLIED, 2, 0, "instruction_CLC", &Cpu6502::instruction_CLC},
		{0x19, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 1, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x1a, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x1b, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x1c, AddressingMode::IMPLIED, 0, 1, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x1d, AddressingMode::INDEXED_ABSOLUTE_X, 4, 1, "instruction_ORA", &Cpu6502::instruction_ORA},
		{0x1e, AddressingMode::INDEXED_ABSOLUTE_X, 7, 0, "instruction_ASL", &Cpu6502::instruction_ASL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x20, AddressingMode::ABSOLUTE, 6, 0, "instruction_JSR", &Cpu6502::instruction_JSR},
		{0x21, AddressingMode::INDEXED_INDIRECT, 6, 0, "instruction_AND", &Cpu6502::instruction_AND},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x24, AddressingMode::ZERO_PAGE, 3, 0, "instruction_BIT", &Cpu6502::instruction_BIT},
		{0x25, AddressingMode::ZERO_PAGE, 3, 0, "instruction_AND", &Cpu6502::instruction_AND},
		{0x26, AddressingMode::ZERO_PAGE, 5, 0, "instruction_ROL", &Cpu6502::instruction_ROL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x28, AddressingMode::IMPLIED, 4, 0, "instruction_PLP", &Cpu6502::instruction_PLP},
		{0x29, AddressingMode::IMMEDIATE, 2, 0, "instruction_AND", &Cpu6502::instruction_AND},
		{0x2a, AddressingMode::ACCUMULATOR, 2, 0, "instruction_ROL", &Cpu6502::instruction_ROL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x2c, AddressingMode::ABSOLUTE, 4, 0, "instruction_BIT", &Cpu6502::instruction_BIT},
		{0x2d, AddressingMode::ABSOLUTE, 4, 0, "instruction_AND", &Cpu6502::instruction_AND},
		{0x2e, AddressingMode::ABSOLUTE, 6, 0, "instruction_ROL", &Cpu6502::instruction_ROL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x30, AddressingMode::RELATIVE, 2, 0, "instruction_BMI", &Cpu6502::instruction_BMI},
		{0x31, AddressingMode::INDIRECT_INDEXED, 5, 1, "instruction_AND", &Cpu6502::instruction_AND},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x35, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_AND", &Cpu6502::instruction_AND},
		{0x36, AddressingMode::INDEXED_ZERO_PAGE_X, 6, 0, "instruction_ROL", &Cpu6502::instruction_ROL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x38, AddressingMode::IMPLIED, 2, 0, "instruction_SEC", &Cpu6502::instruction_SEC},
		{0x39, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 1, "instruction_AND", &Cpu6502::instruction_AND},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x3d, AddressingMode::INDEXED_ABSOLUTE_X, 4, 1, "instruction_AND", &Cpu6502::instruction_AND},
		{0x3e, AddressingMode::INDEXED_ABSOLUTE_X, 7, 0, "instruction_ROL", &Cpu6502::instruction_ROL},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x40, AddressingMode::IMPLIED, 6, 0, "instruction_RTI", &Cpu6502::instruction_RTI},
		{0x41, AddressingMode::INDEXED_INDIRECT, 5, 0, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x45, AddressingMode::ZERO_PAGE, 3, 0, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0x46, AddressingMode::ZERO_PAGE, 5, 0, "instruction_LSR", &Cpu6502::instruction_LSR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x48, AddressingMode::IMPLIED, 3, 0, "instruction_PHA", &Cpu6502::instruction_PHA},
		{0x49, AddressingMode::IMMEDIATE, 2, 0, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0x4a, AddressingMode::ACCUMULATOR, 2, 0, "instruction_LSR", &Cpu6502::instruction_LSR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x4c, AddressingMode::ABSOLUTE, 3, 0, "instruction_JMP", &Cpu6502::instruction_JMP},
		{0x4d, AddressingMode::ABSOLUTE, 4, 0, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0x4e, AddressingMode::ABSOLUTE, 6, 0, "instruction_LSR", &Cpu6502::instruction_LSR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x50, AddressingMode::RELATIVE, 2, 0, "instruction_BVC", &Cpu6502::instruction_BVC},
		{0x51, AddressingMode::INDIRECT_INDEXED, 6, 1, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x55, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0x56, AddressingMode::INDEXED_ZERO_PAGE_X, 6, 0, "instruction_LSR", &Cpu6502::instruction_LSR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x58, AddressingMode::IMPLIED, 2, 0, "instruction_CLI", &Cpu6502::instruction_CLI},
		{0x59, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 1, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x5d, AddressingMode::INDEXED_ABSOLUTE_X, 4, 1, "instruction_EOR", &Cpu6502::instruction_EOR},
		{0x5e, AddressingMode::INDEXED_ABSOLUTE_X, 7, 0, "instruction_LSR", &Cpu6502::instruction_LSR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x60, AddressingMode::IMPLIED, 6, 0, "instruction_RTS", &Cpu6502::instruction_RTS},
		{0x61, AddressingMode::INDEXED_INDIRECT, 6, 0, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x65, AddressingMode::ZERO_PAGE, 3, 0, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0x66, AddressingMode::ZERO_PAGE, 5, 0, "instruction_ROR", &Cpu6502::instruction_ROR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x68, AddressingMode::IMPLIED, 4, 0, "instruction_PLA", &Cpu6502::instruction_PLA},
		{0x69, AddressingMode::IMMEDIATE, 2, 0, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0x6a, AddressingMode::ACCUMULATOR, 2, 0, "instruction_ROR", &Cpu6502::instruction_ROR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x6c, AddressingMode::INDIRECT, 5, 0, "instruction_JMP", &Cpu6502::instruction_JMP},
		{0x6d, AddressingMode::ABSOLUTE, 4, 0, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0x6e, AddressingMode::ABSOLUTE, 6, 0, "instruction_ROR", &Cpu6502::instruction_ROR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x70, AddressingMode::RELATIVE, 2, 0, "instruction_BVS", &Cpu6502::instruction_BVS},
		{0x71, AddressingMode::INDIRECT_INDEXED, 5, 1, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x75, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0x76, AddressingMode::INDEXED_ZERO_PAGE_X, 6, 0, "instruction_ROR", &Cpu6502::instruction_ROR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x78, AddressingMode::IMPLIED, 2, 0, "instruction_SEI", &Cpu6502::instruction_SEI},
		{0x79, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 1, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x7d, AddressingMode::INDEXED_ABSOLUTE_X, 4, 1, "instruction_ADC", &Cpu6502::instruction_ADC},
		{0x7e, AddressingMode::INDEXED_ABSOLUTE_X, 7, 0, "instruction_ROR", &Cpu6502::instruction_ROR},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x81, AddressingMode::INDEXED_INDIRECT, 6, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x84, AddressingMode::ZERO_PAGE, 3, 0, "instruction_STY", &Cpu6502::instruction_STY},
		{0x85, AddressingMode::ZERO_PAGE, 3, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0x86, AddressingMode::ZERO_PAGE, 3, 0, "instruction_STX", &Cpu6502::instruction_STX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x88, AddressingMode::IMPLIED, 2, 0, "instruction_DEY", &Cpu6502::instruction_DEY},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x8a, AddressingMode::IMPLIED, 2, 0, "instruction_TXA", &Cpu6502::instruction_TXA},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x8c, AddressingMode::ABSOLUTE, 4, 0, "instruction_STY", &Cpu6502::instruction_STY},
		{0x8d, AddressingMode::ABSOLUTE, 4, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0x8e, AddressingMode::ABSOLUTE, 4, 0, "instruction_STX", &Cpu6502::instruction_STX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x90, AddressingMode::RELATIVE, 2, 0, "instruction_BCC", &Cpu6502::instruction_BCC},
		{0x91, AddressingMode::INDIRECT_INDEXED, 6, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x94, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_STY", &Cpu6502::instruction_STY},
		{0x95, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0x96, AddressingMode::INDEXED_ZERO_PAGE_Y, 4, 0, "instruction_STX", &Cpu6502::instruction_STX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x98, AddressingMode::IMPLIED, 2, 0, "instruction_TYA", &Cpu6502::instruction_TYA},
		{0x99, AddressingMode::INDEXED_ABSOLUTE_Y, 5, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0x9a, AddressingMode::IMPLIED, 2, 0, "instruction_TXS", &Cpu6502::instruction_TXS},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0x9d, AddressingMode::INDEXED_ABSOLUTE_X, 5, 0, "instruction_STA", &Cpu6502::instruction_STA},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xa0, AddressingMode::IMMEDIATE, 2, 0, "instruction_LDY", &Cpu6502::instruction_LDY},
		{0xa1, AddressingMode::INDEXED_INDIRECT, 6, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xa2, AddressingMode::IMMEDIATE, 2, 0, "instruction_LDX", &Cpu6502::instruction_LDX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xa4, AddressingMode::ZERO_PAGE, 3, 0, "instruction_LDY", &Cpu6502::instruction_LDY},
		{0xa5, AddressingMode::ZERO_PAGE, 3, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xa6, AddressingMode::ZERO_PAGE, 3, 0, "instruction_LDX", &Cpu6502::instruction_LDX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xa8, AddressingMode::IMPLIED, 2, 0, "instruction_TAY", &Cpu6502::instruction_TAY},
		{0xa9, AddressingMode::IMMEDIATE, 2, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xaa, AddressingMode::IMPLIED, 2, 0, "instruction_TAX", &Cpu6502::instruction_TAX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xac, AddressingMode::ABSOLUTE, 4, 0, "instruction_LDY", &Cpu6502::instruction_LDY},
		{0xad, AddressingMode::ABSOLUTE, 4, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xae, AddressingMode::ABSOLUTE, 4, 0, "instruction_LDX", &Cpu6502::instruction_LDX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xb0, AddressingMode::RELATIVE, 2, 0, "instruction_BCS", &Cpu6502::instruction_BCS},
		{0xb1, AddressingMode::INDIRECT_INDEXED, 5, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xb4, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_LDY", &Cpu6502::instruction_LDY},
		{0xb5, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xb6, AddressingMode::INDEXED_ZERO_PAGE_Y, 4, 0, "instruction_LDX", &Cpu6502::instruction_LDX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xb8, AddressingMode::IMPLIED, 2, 0, "instruction_CLV", &Cpu6502::instruction_CLV},
		{0xb9, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xba, AddressingMode::IMPLIED, 2, 0, "instruction_TSX", &Cpu6502::instruction_TSX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xbc, AddressingMode::INDEXED_ABSOLUTE_X, 4, 0, "instruction_LDY", &Cpu6502::instruction_LDY},
		{0xbd, AddressingMode::INDEXED_ABSOLUTE_X, 4, 0, "instruction_LDA", &Cpu6502::instruction_LDA},
		{0xbe, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 0, "instruction_LDX", &Cpu6502::instruction_LDX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xc0, AddressingMode::IMMEDIATE, 2, 0, "instruction_CPY", &Cpu6502::instruction_CPY},
		{0xc1, AddressingMode::INDEXED_INDIRECT, 5, 0, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xc4, AddressingMode::ZERO_PAGE, 3, 0, "instruction_CPY", &Cpu6502::instruction_CPY},
		{0xc5, AddressingMode::ZERO_PAGE, 3, 0, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0xc6, AddressingMode::ZERO_PAGE, 5, 0, "instruction_DEC", &Cpu6502::instruction_DEC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xc8, AddressingMode::IMPLIED, 2, 0, "instruction_INY", &Cpu6502::instruction_INY},
		{0xc9, AddressingMode::IMMEDIATE, 2, 0, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0xca, AddressingMode::IMPLIED, 2, 0, "instruction_DEX", &Cpu6502::instruction_DEX},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xcc, AddressingMode::ABSOLUTE, 4, 0, "instruction_CPY", &Cpu6502::instruction_CPY},
		{0xcd, AddressingMode::ABSOLUTE, 4, 0, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0xce, AddressingMode::ABSOLUTE, 6, 0, "instruction_DEC", &Cpu6502::instruction_DEC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xd0, AddressingMode::RELATIVE, 2, 0, "instruction_BNE", &Cpu6502::instruction_BNE},
		{0xd1, AddressingMode::INDIRECT_INDEXED, 6, 1, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xd5, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0xd6, AddressingMode::INDEXED_ZERO_PAGE_X, 6, 0, "instruction_DEC", &Cpu6502::instruction_DEC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xd8, AddressingMode::IMPLIED, 2, 0, "instruction_CLD", &Cpu6502::instruction_CLD},
		{0xd9, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 1, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xdd, AddressingMode::INDEXED_ABSOLUTE_X, 4, 1, "instruction_CMP", &Cpu6502::instruction_CMP},
		{0xde, AddressingMode::INDEXED_ABSOLUTE_X, 7, 0, "instruction_DEC", &Cpu6502::instruction_DEC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xe0, AddressingMode::IMMEDIATE, 2, 0, "instruction_CPX", &Cpu6502::instruction_CPX},
		{0xe1, AddressingMode::INDEXED_INDIRECT, 5, 0, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xe4, AddressingMode::ZERO_PAGE, 3, 0, "instruction_CPX", &Cpu6502::instruction_CPX},
		{0xe5, AddressingMode::ZERO_PAGE, 3, 0, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0xe6, AddressingMode::ZERO_PAGE, 5, 0, "instruction_INC", &Cpu6502::instruction_INC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xe8, AddressingMode::IMPLIED, 2, 0, "instruction_INX", &Cpu6502::instruction_INX},
		{0xe9, AddressingMode::IMMEDIATE, 2, 0, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0xea, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xec, AddressingMode::ABSOLUTE, 4, 0, "instruction_CPX", &Cpu6502::instruction_CPX},
		{0xed, AddressingMode::ABSOLUTE, 4, 0, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0xee, AddressingMode::ABSOLUTE, 6, 0, "instruction_INC", &Cpu6502::instruction_INC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xf0, AddressingMode::RELATIVE, 2, 0, "instruction_BEQ", &Cpu6502::instruction_BEQ},
		{0xf1, AddressingMode::INDIRECT_INDEXED, 6, 1, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xf5, AddressingMode::INDEXED_ZERO_PAGE_X, 4, 0, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0xf6, AddressingMode::INDEXED_ZERO_PAGE_X, 6, 0, "instruction_INC", &Cpu6502::instruction_INC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xf8, AddressingMode::IMPLIED, 2, 0, "instruction_SED", &Cpu6502::instruction_SED},
		{0xf9, AddressingMode::INDEXED_ABSOLUTE_Y, 4, 1, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP},
		{0xfd, AddressingMode::INDEXED_ABSOLUTE_X, 4, 1, "instruction_SBC", &Cpu6502::instruction_SBC},
		{0xfe, AddressingMode::INDEXED_ABSOLUTE_X, 7, 0, "instruction_INC", &Cpu6502::instruction_INC},
		{0, AddressingMode::IMPLIED, 2, 0, "instruction_NOP", &Cpu6502::instruction_NOP}
};

int Cpu6502::step_one() {
	int extra_cy = 0;
	if (this->penalty) {
		this->penalty--;
		return 0;
	}
	if (this->nmi_pending) {
//		printf("NMI\n");
		instruction_BRK(AddressingMode::IMPLIED);
		this->nmi_pending = false;
		extra_cy = 7;
	}

	Instruction instruction_info = instruction_map[this->nes->mem()->read_address(this->PC)];

	if (instruction_info.cycles) { // If a valid instruction
		this->current_instruction = this->PC;
		this->PC += get_size(instruction_info.mode);
		
		if (false) {
			int size = get_size(instruction_info.mode);
			std::cout << instruction_info.name << ":" << std::endl;
			printf("%.4X  ", (unsigned int)this->current_instruction);

			for (int i = 0; i < 3; i++) {
				if (size > i)
					printf("%.2X ", (unsigned int)this->nes->mem()->read_address(this->current_instruction+i));
				else
					printf("   ");
			
			}
			printf("A:%.2X ", (unsigned int)this->A);
			printf("X:%.2X ", (unsigned int)this->X);
			printf("Y:%.2X ", (unsigned int)this->Y);
			printf("P:%.2X ", (unsigned int)this->S);
			printf("SP:%.2X\n", (unsigned int)this->SP);
		}

		int ret = (this->*instruction_info.function)(instruction_info.mode) + instruction_info.cycles + extra_cy;
		if (this->needs_crossed_page_cycles)
			ret += instruction_info.pageCycles;
//		printf("%.4X  ", (unsigned int)this->current_instruction);
//		printf("CYCL:  %X\n", ret);
//		if (this->current_instruction == 0x81d3)
//			printf("%.4X %X\n", (unsigned int)this->nes->mem()->read_address16(this->current_instruction+1), (unsigned int)this->X);
		return ret;
	
	}

	std::cout << "INVALID INSTRUCTION" << std::hex <<  (int)this->nes->mem()->read_address(this->PC) << std::endl;
	this->PC += 1;
	abort();
	return 0;
}
