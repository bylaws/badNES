#pragma once

#include <stdint.h> 

class NES;

class Ppu2c02 {
	public:
		Ppu2c02(NES *nes);
		void run_cycle(void);
		void write(uint16_t address, uint8_t value);
		uint8_t read(uint16_t address);
		void set_mirroring(int type);
	
	private:
		NES *nes;

		void write_control_flags(uint8_t value);
		void write_mask_flags(uint8_t value);
		uint8_t read_status_flags();
		uint16_t mirror_address(uint16_t address);
		uint8_t read_direct(uint16_t address);
		void nmi_update(void);
		uint8_t bg_pixel(void);
		uint8_t sprite_pixel(uint8_t &index);
		void draw_new_pixel(void);

		uint8_t *vram; //= np.zeros(0x8000, dtype=np.uint8)
		uint16_t nametable0_offset; //=0
		uint16_t nametable1_offset; //=0
		uint16_t nametable2_offset; //=0
		uint16_t nametable3_offset; //=0
		uint8_t *sprite;// = np.zeros(0x100, dtype=np.uint8)
		uint8_t *palette; // = np.zeros(0x20, dtype=np.uint8)
		int sprite_cnt;
		uint32_t *sprite_pattern;
		uint8_t *sprite_pos;
		uint8_t *sprite_pri;
		uint8_t *sprite_idx;
		bool ctrl_nmi_enable; //=0
		bool ctrl_sprite_size; //=0
		bool ctrl_bg_pattern_address; //=0
		bool ctrl_sprite_pattern_address; //=0
		bool ctrl_vram_increment; //=0
		bool ctrl_nametable_base; //=0
		bool mask_greyscale; //=0
		bool mask_bg_leftmost; //=0
		bool mask_sprites_leftmost; //=0
		bool mask_sprites; //=0
		bool mask_bg; //=0
		bool mask_emph_r; //=0
		bool mask_emph_g; //=0
		bool mask_emph_b; //=0
		bool stat_overflow; //=0
		bool stat_sprite_zero_hit; //=0
		bool stat_in_vblank; //=0
		uint8_t latch_reg; //=0
		uint8_t oam_addr_reg; //=0
		uint16_t vram_addr_reg; //=0
		bool odd_frame; //=0
		unsigned int frame; //=0
		int scanline; //=0
		uint64_t tile_data; //=0
		int cycle; //=0
		unsigned int nmi_delay; //=0
		bool nmi_occured; //=0
		bool nmi_previous; //=0
		uint8_t attribute_table_byte; //=0
		uint8_t nametable_byte; //=0
		uint8_t high_tile_byte; //=0
		uint8_t low_tile_byte; //=0
		uint8_t x; //=0
		bool w; //=0
		uint16_t t; //=0
		uint8_t buffer; //=0
};
