#include "ppu.h"
#include "nes.h"
#include <iostream>
#include <stdio.h>

Ppu2c02::Ppu2c02(NES *nes) {
	this->nes = nes;
	this->vram = new uint8_t[0x8000]();
	this->nametable0_offset = 0;
	this->nametable1_offset = 0x400;
	this->nametable2_offset = 0;
	this->nametable3_offset = 0x400;
	this->sprite = new uint8_t[0x100]();
	this->palette = new uint8_t[0x20]();
	this->sprite_cnt = 0;
	this->sprite_pattern = new uint32_t[8]();
	this->sprite_pos = new uint8_t[8]();
	this->sprite_pri = new uint8_t[8]();
	this->sprite_idx = new uint8_t[8]();
	this->ctrl_nmi_enable = false;
	this->ctrl_sprite_size = false;
	this->ctrl_bg_pattern_address = false;
	this->ctrl_sprite_pattern_address = false;
	this->ctrl_vram_increment = false;
	this->ctrl_nametable_base = false;
	this->mask_greyscale = false;
	this->mask_bg_leftmost = false;
	this->mask_sprites_leftmost = false;
	this->mask_sprites = false;
	this->mask_bg = false;
	this->mask_emph_r = false;
	this->mask_emph_g = false;
	this->mask_emph_b = false;
	this->stat_overflow = false;
	this->stat_sprite_zero_hit = true;
	this->stat_in_vblank = false;
	this->latch_reg = 0;
	this->oam_addr_reg = 0;
	this->vram_addr_reg = 0;
	this->odd_frame = false;
	this->frame = 0;
	this->scanline = 240;
	this->tile_data = 0;
	this->cycle = 340;
	this->nmi_delay = 0;
	this->nmi_occured = false;
	this->nmi_previous = true;
	this->attribute_table_byte = 0;
	this->nametable_byte = 0;
	this->high_tile_byte = 0;
	this->low_tile_byte = 0;
	this->x = 0;
	this->w = false;
	this->t = 0;
	this->buffer = 0;


}

void Ppu2c02::write_control_flags(uint8_t value) {
	this->ctrl_nametable_base = value & 0b11;
	this->ctrl_vram_increment = (value >> 2) & 1;
	this->ctrl_sprite_pattern_address = (value >> 3) & 1;
	this->ctrl_bg_pattern_address = (value >> 4) & 1;
	this->ctrl_sprite_size = (value >> 5) & 1;
	this->ctrl_nmi_enable = (value >> 7) & 1;
	nmi_update();
	this->t = (this->t & 0xF3FF) | (((uint16_t)(value) & 0x03) << 10);
}

void Ppu2c02::write_mask_flags(uint8_t value) {
	this->mask_greyscale = value & 1;
	this->mask_bg_leftmost = (value >> 1) & 1;
	this->mask_sprites_leftmost = (value >> 2) & 1;
	this->mask_bg = (value >> 3) & 1;
	this->mask_sprites = (value >> 4) & 1;
	this->mask_emph_r = (value >> 5) & 1;
	this->mask_emph_g = (value >> 6) & 1;
	this->mask_emph_b = (value >> 7) & 1;
}

uint8_t Ppu2c02::read_status_flags() {
	uint8_t value = this->latch_reg & 0b11111;
	value |= this->stat_overflow << 5;
	value |= this->stat_sprite_zero_hit << 6;
	if (this->nmi_occured)
		value |= 1 << 7;
	this->nmi_occured = false;
	this->nmi_update();

	this->w = false;

	return value;
}

void Ppu2c02::set_mirroring(int type) {
	switch (type) {
		case 3:
			this->nametable0_offset = this->nametable1_offset = 0;
			this->nametable1_offset = this->nametable2_offset = 0x400;
		case 2:
			this->nametable0_offset = this->nametable2_offset = 0;
			this->nametable1_offset = this->nametable3_offset = 0x400;
		case 1:
			this->nametable0_offset = this->nametable2_offset = this->nametable1_offset = this->nametable3_offset = 0;
		case 0:
			this->nametable0_offset = this->nametable2_offset = this->nametable1_offset = this->nametable3_offset = 0x400;
		default:
			return;
	}
}
uint16_t Ppu2c02::mirror_address(uint16_t address) {
	if (address < 0x2400)
		return this->nametable0_offset + (address & 0x3ff);
	else if (address < 0x2800)
		return this->nametable1_offset + (address & 0x3ff);
	else if (address < 0x2c00)
		return this->nametable2_offset + (address & 0x3ff);
	else
		return this->nametable3_offset + (address & 0x3ff);
}

uint8_t Ppu2c02::read_direct(uint16_t address) {
	address = address % 0x4000;
	if (address < 0x2000)
		return this->nes->mem()->mapper()->read(address);
	else if (address < 0x3f00)
		return this->vram[mirror_address(address)];
	else if (address < 0x4000) {
		address = address % 32;
		if (address >= 16 && address % 4 == 0)
			address -= 16;

		return this->palette[address];
	} else
		return 0;
}


uint8_t Ppu2c02::read(uint16_t address) {
	if (address == 0x2002)
		return this->read_status_flags();
	else if (address == 0x2004)
		return this->sprite[oam_addr_reg];
	else if (address == 0x2007) {
		uint8_t ret = 0;
		uint16_t vram_address = this->vram_addr_reg % 0x4000;
		if (vram_address < 0x2000) {
			ret = this->buffer;
			this->buffer = this->nes->mem()->mapper()->read(vram_address);
		} else if (vram_address < 0x3f00) {
			ret = this->buffer;
			this->buffer = this->vram[mirror_address(vram_address)];
		} else if (vram_address < 0x4000) {
			vram_address = vram_address % 32;
			if (vram_address >= 16 && vram_address % 4 == 0)
				vram_address -= 16;

			this->buffer = this->read_direct(this->vram_addr_reg - 0x1000);
			ret = this->palette[vram_address];
		}

		if (this->ctrl_vram_increment)
			this->vram_addr_reg += 32;
		else
			this->vram_addr_reg += 1;

		return ret;
	}
	return 0;
}

void Ppu2c02::write(uint16_t address, uint8_t value) {
	this->latch_reg = value;
	if (address == 0x2000) {
		write_control_flags(value);
		return;
	} else if (address == 0x2001) {
		write_mask_flags(value);
		return;
	} else if (address == 0x2003) {
		oam_addr_reg = value;
		return;
	} else if (address == 0x2004) {
		this->sprite[this->oam_addr_reg] = value;
		this->oam_addr_reg += 1;
		return;
	} else if (address == 0x2005) {
		if (this->w == false) {
			this->t = (this->t & 0xFFE0) | (value >> 3);
			this->x = value & 0x07;
			this->w = true;
		} else {
			this->t = (this->t & 0x8FFF) | ((uint16_t)(value & 0x07) << 12);
			this->t = (this->t & 0xFC1F) | ((uint16_t)(value & 0xF8) << 2);
			this->w = false;
		}
		return;
	} else if (address == 0x2006) {
		if (this->w == false) {
			this->t = (this->t & 0x80FF) | ((uint16_t)(value & 0x3F) << 8);
			this->w = true;
		} else {
			this->t = (this->t & 0xFF00) | (uint16_t)value;
			this->vram_addr_reg = this->t;
			this->w = false;
		}
		return;
	} else if (address == 0x2007) {
		uint16_t vram_address = this->vram_addr_reg % 0x4000;
		if (vram_address < 0x2000) {
			this->nes->mem()->mapper()->write(vram_address, value);
		} else if (vram_address < 0x3f00) {
			this->vram[mirror_address(vram_address)] = value;
		} else if (vram_address < 0x4000) {
			vram_address = vram_address % 32;
			if (vram_address >= 16 && vram_address % 4 == 0)
				vram_address -= 16;

			this->palette[vram_address] = value;
		}

		if (this->ctrl_vram_increment)
			this->vram_addr_reg += 32;
		else
			this->vram_addr_reg += 1;

		return;
	} else if (address == 0x4014) {
		uint16_t page = value << 8;
//		printf("HEO %llu\n", this->nes->cycles);
		if (this->nes->cycles % 2 != 1)
			this->nes->cpu()->add_penalty(513);
		else {
//			printf("HE\n");
			this->nes->cpu()->add_penalty(514);
		}

		for (int i = 0; i < 256; i++) {
			this->sprite[this->oam_addr_reg] = this->nes->mem()->ram[page + i];
			this->oam_addr_reg = (this->oam_addr_reg + 1) & 0xff;
		}
	}
}

void Ppu2c02::nmi_update() {
	bool nmi = this->nmi_occured && this->ctrl_nmi_enable;
	if (nmi && !this->nmi_previous)
		this->nmi_delay = 15;
	this->nmi_previous = nmi;
}

uint8_t Ppu2c02::sprite_pixel(uint8_t &index) {
	int offset;
	uint8_t color = 0;
	for (int i = 0; i < this->sprite_cnt; i++) {
		offset = (this->cycle - 1) - (int)this->sprite_pos[i];

		if (offset < 0 || offset > 7) // Sprite isnt in range of the x coords we are currently drawing
			continue;

		offset = 7 - offset; // we store in reverse order
		color = ((this->sprite_pattern[i] >> (offset * 4)) & 0xf);

		if (color%4 == 0)
			continue;
		
		index = i; // Set reference
//		std::cout << "offset: " << offset << " color :" << (int)color << std::endl;
		return color;
	}
	return 0;
}
void Ppu2c02::draw_new_pixel() {
	int xc = this->cycle - 1;
	int y = this->scanline;
	uint8_t background=0, sprite=0, i = 0;

	if (this->mask_bg)
		background = (uint8_t)((((uint32_t)((this->tile_data >> 31) >> 1) >> ((7-this->x)*4))) & 0x0f);
	
	if (this->mask_sprites)
		sprite = sprite_pixel(i);

	if (xc < 8 && this->mask_sprites_leftmost == 0)
		sprite = 0;
	if (xc < 8 && this->mask_bg_leftmost == 0)
		background = 0;

	uint8_t color = 0;

	bool bg = background % 4 != 0;
	bool sp = sprite % 4 != 0;

	if (!bg && !sp) {
		color = 0;
	} else if (!bg && sp) {
		color = sprite | (uint8_t)0x10;
	} else if (bg && !sp) {
		color = background;
	} else if (bg && sp) {
		if (this->sprite_idx[i]== 0 && xc < 255)
			this->stat_sprite_zero_hit = true;
		if (this->sprite_pri[i] == 0)
			color = sprite | (uint8_t)0x10;
		else
			color = background;
	}
	if (color >= 16 && color % 4 == 0)
		color -= 16;

	color = this->palette[color] % 64;
	this->nes->draw_pixel(xc, y, color);
//	this->image[y, x] = list(palette_conv[this->palette[color]])
}



void Ppu2c02::run_cycle() {
//	print("Start PPU:")
//	printf("%d %d\n", this->scanline, this->cycle);
//	std::cout << "ppu->cycle"<< " " << (int)this->cycle << std::endl;
//	std::cout << "vram_addr_reg: " << (unsigned int)this->vram_addr_reg << std::endl;
//	print("this->cycle", this->cycle, end = " ")
//	print("this->frame", this->frame)
//	print("this->odd_frame", this->odd_frame)
//	print("this->vram_addr_reg", this->vram_addr_reg)
	if (this->nmi_delay) {
		this->nmi_delay -= 1;
		if (this->nmi_delay == 0 && this->ctrl_nmi_enable && this->nmi_occured)
			this->nes->cpu()->trigger_nmi();
	}

	bool start_frame = false;
	if (this->mask_bg || this->mask_sprites) {
		if (this->odd_frame && this->scanline == 261 && this->cycle == 339) {
			this->cycle = 0;
			this->scanline = 0;
			this->frame += 1;
			this->odd_frame = !this->odd_frame;
			start_frame = true;
		}
	}

	if (!start_frame) {
		this->cycle += 1;
		if (this->cycle > 340) { //Next row
			this->cycle = 0;
			this->scanline += 1;
			if (this->scanline > 261) {
				this->scanline = 0;
				this->frame += 1;
				this->odd_frame = !this->odd_frame;
			}
		}
	}

	// from foglemans emu
	bool preLine = this->scanline == 261;
	bool visibleLine = this->scanline < 240;
	// postLine := ppu.ScanLine == 240
	bool renderLine = preLine || visibleLine;
	bool preFetchCycle = this->cycle >= 321 && this->cycle <= 336;
	bool visibleCycle = this->cycle >= 1 && this->cycle <= 256;
	bool fetchCycle = preFetchCycle || visibleCycle;

	if (this->mask_bg || this->mask_sprites) { //If drawing anything
		if (visibleLine && visibleCycle)
			draw_new_pixel();

		if (renderLine && fetchCycle) {
			this->tile_data = (this->tile_data << 4);
//			printf("%llX\n", tile_data);
			uint16_t address;
			uint16_t shift;
			uint8_t fine_y;
			uint32_t tmp_data;
			switch (this->cycle % 8) {
				case 1 : this->nametable_byte = read_direct(0x2000 | (this->vram_addr_reg & 0x0fff));
						 break;
				case 3 : address = 0x23C0 | (this->vram_addr_reg & 0x0C00) | ((this->vram_addr_reg >> 4) & 0x38) | ((this->vram_addr_reg >> 2) & 0x07);
						 shift = ((this->vram_addr_reg >> 4) & 4) | (this->vram_addr_reg & 2);
						 this->attribute_table_byte = (((read_direct(address) >> shift) & 3) << 2);
						 break;
				case 5 : fine_y = (this->vram_addr_reg >> 12) & 7;
						 address = (0x1000 * (uint16_t)this->ctrl_bg_pattern_address) + ((uint16_t)this->nametable_byte*16 + fine_y);
//				std::cout << "fine_y: " << fine_y << " address: " << address << std::endl;
						 this->low_tile_byte = this->nes->mem()->mapper()->read(address & 0xffff);
						 break;
				case 7 : fine_y = (this->vram_addr_reg >> 12) & 7;
						 address = (0x1000 * (uint16_t)this->ctrl_bg_pattern_address) + ((uint16_t)this->nametable_byte*16 + fine_y);
						 this->high_tile_byte = this->nes->mem()->mapper()->read((address + 8) & 0xffff);
						 break;
				case 0 : tmp_data = 0;
//						 std::cout << "attribute_byte: " << (int)this->attribute_table_byte << std::endl; 
//						 std::cout << "low_tile_byte: " << (int)this->low_tile_byte << std::endl; 
//						 std::cout << "high_tile_byte: " << (int)this->high_tile_byte << std::endl; 
						 for (int i = 0; i < 8; i++) {
							 tmp_data = tmp_data << 4;
//							 std::cout << "tmp: " << std::hex << tmp_data << std::endl; 
							 tmp_data |= (this->attribute_table_byte | ((this->low_tile_byte & 0x80) >> 7) | ((this->high_tile_byte & 0x80) >> 6)) & 0xffffffff;
//							 std::cout << "tmp2: " << std::hex << tmp_data << std::endl; 
							 this->low_tile_byte = this->low_tile_byte << 1;
							 this->high_tile_byte = this->high_tile_byte << 1;
						 }
						 this->tile_data |= tmp_data;
//					 	std::cout << "tile_data: " << this->tile_data << std::endl; 
			}
		}
		if (preLine && this->cycle >= 280 && this->cycle <= 304) {
			this->vram_addr_reg = (this->vram_addr_reg & 0x841f) | (this->t & 0x7be0);
//			std::cout << "t: " << (unsigned int)this->t << std::endl;
		}
		if (renderLine) {
			if (fetchCycle && this->cycle%8 == 0) {
				if ((this->vram_addr_reg & (uint16_t)0x001f) == 31) {
					this->vram_addr_reg &= 0xffe0;
					this->vram_addr_reg ^= 0x0400;
				} else {
					this->vram_addr_reg = (this->vram_addr_reg + 1)&0xffff;
				}
			}
			if (this->cycle == 256) {
				if ((this->vram_addr_reg & (uint16_t)0x7000) == 0x7000) {
					this->vram_addr_reg &= 0x8fff;
					uint16_t y = (this->vram_addr_reg & 0x03e0) >> 5;

					if (y == 29) { //Switch nametable
						y = 0;
						this->vram_addr_reg ^= 0x800;
					} else if (y == 31) {
						y = 0;
					} else {
						y += 1;
					}
					this->vram_addr_reg = (this->vram_addr_reg & 0xfc1f) | (y << 5);
				} else {
					this->vram_addr_reg += 0x1000;
				}
			} else if (this->cycle == 257) {
				this->vram_addr_reg = (this->vram_addr_reg & 0xFBE0) | (this->t & 0x041F);
			}

		}

		if (this->cycle == 257) {
			if (this->scanline < 240) {
				int row;
				int height = (this->ctrl_sprite_size+1)*8;
				uint8_t attribute;
				uint8_t tile;
				uint16_t address;
				uint32_t tmp_data;
				uint8_t color;
				uint32_t tmp_low_tile_byte; // Each tile is split into two bytes.
				uint32_t tmp_high_tile_byte; // They each bit of low is added to the same of high.
				// This then produces a index for the color in the selected palette
				this->sprite_cnt = 0;
				for (int i = 0; i < 64; i++) {
					row = this->scanline - (int)this->sprite[(4*i)+0]; // Y pos of top of the sprite. (each sprite in oam is 4 bytes)
					if (row < 0 || row >= height)
						continue;

					if (this->sprite_cnt < 8) {
						attribute = this->sprite[(4*i)+2];
						tile = this->sprite[(4*i)+1];

						if (!this->ctrl_sprite_size) {
							if ((attribute & (uint8_t)0x80) == 0x80)
								row =  7 - row;
							
							bool table = this->ctrl_sprite_pattern_address;
							address = (0x1000 * table) + ((uint16_t)tile)*16 + (uint16_t)row;
						} else {
							if ((attribute & (uint8_t)0x80) == 0x80)
								row =  15 - row;
						
							bool table = tile & 1;
							tile &= 0xfe;

							if (row > 7) {
								tile++;
								row -= 8;
							}

							address = (0x1000 * table) + ((uint16_t)tile)*16 + (uint16_t)row;
						}
						
						color = (attribute & (uint8_t)3) << 2; // Number or palette wanted
						tmp_low_tile_byte = this->nes->mem()->mapper()->read(address & 0xffff);
						tmp_high_tile_byte = this->nes->mem()->mapper()->read((address + 8) & 0xffff);
						for (int k = 0; k < 8; k++) {
							tmp_data <<= 4;
							if ((uint8_t)((uint8_t)attribute & (uint8_t)0x40) == (uint8_t)0x40) {
								tmp_data |= (uint32_t)(color | (tmp_low_tile_byte & 1) | ((tmp_high_tile_byte & 1) << 1));
								tmp_low_tile_byte >>= 1;
								tmp_high_tile_byte >>= 1;
							} else {
								tmp_data |= (uint32_t)(color | ((tmp_low_tile_byte & 0x80) >> 7) | ((tmp_high_tile_byte & 0x80) >> 6));
								tmp_low_tile_byte <<= 1;
								tmp_high_tile_byte <<= 1;
							}
						}
						this->sprite_pattern[this->sprite_cnt] = tmp_data;
						this->sprite_pos[this->sprite_cnt] = this->sprite[(4*i)+3]; //Set position for current sprite index to X
						this->sprite_pri[this->sprite_cnt] = (attribute >> 5) & 1;
						this->sprite_idx[this->sprite_cnt] = (uint8_t)i;
					}
					this->sprite_cnt++;
				}
				if (this->sprite_cnt > 8) {
					this->sprite_cnt = 8;
					this->stat_overflow = 1;
				}
			} else {
				this->sprite_cnt = 0;
			}
		}
	}

	if (this->scanline == 241 && this->cycle == 1) {
		this->nmi_occured = true;
		nmi_update();
		this->nes->frame_done();
	}

	if (this->cycle == 1 && this->scanline == 261) {
		this->nmi_occured = false;
		nmi_update();
		this->stat_sprite_zero_hit = 0;
		this->stat_overflow = 0;
	}
}
