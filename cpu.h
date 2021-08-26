#pragma once

#include <stdint.h> 
#include <string> 

class NES;

#define STATUS_CARRY (1)
#define STATUS_ZERO (1 << 1)
#define STATUS_IRQ_DISABLE (1 << 2)
#define STATUS_DECIMAL (1 << 3)
#define STATUS_BREAK_MODE (1 << 4)
#define STATUS_RESERVED (1 << 5)
#define STATUS_OVERFLOW (1 << 6)
#define STATUS_NEGATIVE (1 << 7)


enum class AddressingMode {
	ZERO_PAGE = 0,
	INDEXED_ZERO_PAGE_X,
	INDEXED_ZERO_PAGE_Y,
	ABSOLUTE,
	INDEXED_ABSOLUTE_X,
	INDEXED_ABSOLUTE_Y,
	IMPLIED,
	ACCUMULATOR,
	IMMEDIATE,
	RELATIVE,
	INDEXED_INDIRECT,
	INDIRECT_INDEXED,
	INDIRECT
};

class Cpu6502 {
	public:
		Cpu6502(NES *nes);

		int step_one(void);
		
		void reset(void);

		void trigger_nmi(void) {
			this->nmi_pending = true;
		}
		
		void add_penalty(uint16_t p) {
			this->penalty = p;
		}

		uint16_t get_instruction_data(AddressingMode mode);
		int get_size(AddressingMode mode);
		void set_flags(uint8_t byte);
		bool page_crossed(uint16_t a, uint16_t b);
		// Instructions
		int instruction_ADC(AddressingMode mode);
		int instruction_AND(AddressingMode mode);
		int instruction_ASL(AddressingMode mode);
		int instruction_BCC(AddressingMode mode);
		int instruction_BCS(AddressingMode mode);
		int instruction_BEQ(AddressingMode mode);
		int instruction_BIT(AddressingMode mode);
		int instruction_BMI(AddressingMode mode);
		int instruction_BNE(AddressingMode mode);
		int instruction_BPL(AddressingMode mode);
		int instruction_BRK(AddressingMode mode);
		int instruction_BVC(AddressingMode mode);
		int instruction_BVS(AddressingMode mode);
		int instruction_CLC(AddressingMode mode);
		int instruction_CLD(AddressingMode mode);
		int instruction_CLI(AddressingMode mode);
		int instruction_CLV(AddressingMode mode);
		int instruction_CMP(AddressingMode mode);
		int instruction_CPX(AddressingMode mode);
		int instruction_CPY(AddressingMode mode);
		int instruction_DEC(AddressingMode mode);
		int instruction_DEX(AddressingMode mode);
		int instruction_DEY(AddressingMode mode);
		int instruction_EOR(AddressingMode mode);
		int instruction_INC(AddressingMode mode);
		int instruction_INX(AddressingMode mode);
		int instruction_INY(AddressingMode mode);
		int instruction_JMP(AddressingMode mode);
		int instruction_JSR(AddressingMode mode);
		int instruction_LDA(AddressingMode mode);
		int instruction_LDX(AddressingMode mode);
		int instruction_LDY(AddressingMode mode);
		int instruction_LSR(AddressingMode mode);
		int instruction_NOP(AddressingMode mode);
		int instruction_ORA(AddressingMode mode);
		int instruction_PHA(AddressingMode mode);
		int instruction_PHP(AddressingMode mode);
		int instruction_PLA(AddressingMode mode);
		int instruction_PLP(AddressingMode mode);
		int instruction_ROL(AddressingMode mode);
		int instruction_ROR(AddressingMode mode);
		int instruction_RTI(AddressingMode mode);
		int instruction_RTS(AddressingMode mode);
		int instruction_SBC(AddressingMode mode);
		int instruction_SEC(AddressingMode mode);
		int instruction_SED(AddressingMode mode);
		int instruction_SEI(AddressingMode mode);
		int instruction_STA(AddressingMode mode);
		int instruction_STX(AddressingMode mode);
		int instruction_STY(AddressingMode mode);
		int instruction_TAX(AddressingMode mode);
		int instruction_TAY(AddressingMode mode);
		int instruction_TSX(AddressingMode mode);
		int instruction_TXA(AddressingMode mode);
		int instruction_TXS(AddressingMode mode);
		int instruction_TYA(AddressingMode mode);
	private:
		// Registers
		uint8_t A;
		uint8_t X;
		uint8_t Y;
		uint16_t PC; // Program Counter
		uint8_t S; // Status
		uint8_t SP; // Stack Pointer
		uint16_t penalty; // For oam dma
		bool needs_crossed_page_cycles; // For oam dma

		// Misc data
		uint16_t current_instruction;
		bool nmi_pending;
		bool do_sbc;
		NES *nes;

};

struct Instruction {
	uint8_t opcode;
	AddressingMode mode;
	int cycles;
	int pageCycles;
	std::string name;
	int (Cpu6502::*function)(AddressingMode mode);
};
