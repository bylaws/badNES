#include "nes.h"
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <SDL2/SDL.h>

bool KEYS_p1[8];
bool KEYS_p2[8];
NES nes= NES();

uint8_t rowBuffer[400];
uint8_t colBuffer;
char bufferText[256*40*256*2];
unsigned int posa = 0;
bool stop = false;
bool frame = true;

//uint8_t palette_conv_rgb[] = {
//		0x66, 0x66, 0x66, 0x00, 0x2A, 0x88, 0x14, 0x12, 0xA7, 0x3B, 0x00, 0xA4, 0x5C, 0x00, 0x7E, 0x6E, 0x00, 0x40, 0x6C, 0x06, 0x00, 0x56, 0x1D, 0x00,
//		0x33, 0x35, 0x00, 0x0B, 0x48, 0x00, 0x00, 0x52, 0x00, 0x00, 0x4F, 0x08, 0x00, 0x40, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//		0xAD, 0xAD, 0xAD, 0x15, 0x5F, 0xD9, 0x42, 0x40, 0xFF, 0x75, 0x27, 0xFE, 0xA0, 0x1A, 0xCC, 0xB7, 0x1E, 0x7B, 0xB5, 0x31, 0x20, 0x99, 0x4E, 0x00,
//		0x6B, 0x6D, 0x00, 0x38, 0x87, 0x00, 0x0C, 0x93, 0x00, 0x00, 0x8F, 0x32, 0x00, 0x7C, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//		0xFF, 0xFE, 0xFF, 0x64, 0xB0, 0xFF, 0x92, 0x90, 0xFF, 0xC6, 0x76, 0xFF, 0xF3, 0x6A, 0xFF, 0xFE, 0x6E, 0xCC, 0xFE, 0x81, 0x70, 0xEA, 0x9E, 0x22,
//		0xBC, 0xBE, 0x00, 0x88, 0xD8, 0x00, 0x5C, 0xE4, 0x30, 0x45, 0xE0, 0x82, 0x48, 0xCD, 0xDE, 0x4F, 0x4F, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//		0xFF, 0xFE, 0xFF, 0xC0, 0xDF, 0xFF, 0xD3, 0xD2, 0xFF, 0xE8, 0xC8, 0xFF, 0xFB, 0xC2, 0xFF, 0xFE, 0xC4, 0xEA, 0xFE, 0xCC, 0xC5, 0xF7, 0xD8, 0xA5,
//		0xE4, 0xE5, 0x94, 0xCF, 0xEF, 0x96, 0xBD, 0xF4, 0xAB, 0xB3, 0xF3, 0xCC, 0xB5, 0xEB, 0xF2, 0xB8, 0xB8, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t palette_conv_256[] = {
	102, 12, 19, 55, 90, 124, 124, 88, 58, 28, 22, 22, 23, 16, 16, 16, 145, 33, 27, 63, 164, 161, 202, 166, 136, 34, 34, 35, 30, 16, 16, 16, 231, 75,
	69, 105, 213, 204, 209, 215, 214, 154, 77, 84, 44, 102, 16, 16, 231, 153, 147, 183, 219, 217, 223, 223, 222, 192, 157, 158, 14, 225, 16, 16};



void draw_pixel_console(int x, int y, uint8_t color) {
	if (y % 2 && y < 240 && y > 2) {
		if (x % 2) {
			if (frame) {
				posa += snprintf(bufferText + posa, 256, "\x1b[48;5;%dm", palette_conv_256[color]);
				posa += snprintf(bufferText + posa, 256, "\x1b[38;5;%dm", palette_conv_256[rowBuffer[x]]);
			} else {
				posa += snprintf(bufferText + posa, 256, "\x1b[48;5;%dm", palette_conv_256[colBuffer]);
				posa += snprintf(bufferText + posa, 256, "\x1b[38;5;%dm", palette_conv_256[rowBuffer[x-1]]);
			
			}
			if 
			posa += snprintf(bufferText + posa, 256, "▀");
		} else {
			if (x == 0)
				posa += snprintf(bufferText + posa, 256, "\x1b[1B\x1b[500D");

			colBuffer = color;
		}

	} else {
		rowBuffer[x] = color;
	}
}
void frame_done_sdl() {
	frame = !frame;
#ifndef TERMINAL
	SDL_Event event;
	if (SDL_PollEvent(&event)) {
		switch( event.type ){
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym){
				case SDLK_d: KEYS_p1[7] = true; break;
				case SDLK_a: KEYS_p1[6] = true; break;
				case SDLK_w: KEYS_p1[4] = true; break;
				case SDLK_s: KEYS_p1[5] = true; break;
				case SDLK_z: KEYS_p1[2] = true; break; // Select
				case SDLK_x: KEYS_p1[3] = true; break; // Start
				case SDLK_q: KEYS_p1[1] = true; break; // B
				case SDLK_e: KEYS_p1[0] = true; break; // A
				case SDLK_k: KEYS_p2[7] = true; break;
				case SDLK_h: KEYS_p2[6] = true; break;
				case SDLK_u: KEYS_p2[4] = true; break;
				case SDLK_j: KEYS_p2[5] = true; break;
				case SDLK_n: KEYS_p2[2] = true; break; // Select
				case SDLK_m: KEYS_p2[3] = true; break; // Start
				case SDLK_y: KEYS_p2[1] = true; break; // B
				case SDLK_i: KEYS_p2[0] = true; break; // A
			}
			break;

		case SDL_KEYUP:
			switch (event.key.keysym.sym){
				case SDLK_d: KEYS_p1[7] = false; break;
				case SDLK_a: KEYS_p1[6] = false; break;
				case SDLK_w: KEYS_p1[4] = false; break;
				case SDLK_s: KEYS_p1[5] = false; break;
				case SDLK_z: KEYS_p1[2] = false; break; // Select
				case SDLK_x: KEYS_p1[3] = false; break; // Start
				case SDLK_q: KEYS_p1[1] = false; break; // B
				case SDLK_e: KEYS_p1[0] = false; break; // A
				case SDLK_k: KEYS_p2[7] = false; break;
				case SDLK_h: KEYS_p2[6] = false; break;
				case SDLK_u: KEYS_p2[4] = false; break;
				case SDLK_j: KEYS_p2[5] = false; break;
				case SDLK_n: KEYS_p2[2] = false; break; // Select
				case SDLK_m: KEYS_p2[3] = false; break; // Start
				case SDLK_y: KEYS_p2[1] = false; break; // B
				case SDLK_i: KEYS_p2[0] = false; break; // A

			}
			break;

		case SDL_QUIT:
			if (event.type == SDL_QUIT)
				stop = true;
		default:
			break;
		}
	}
	nes.mem()->set_pressed_buttons(false, KEYS_p1);
	nes.mem()->set_pressed_buttons(true, KEYS_p2);
#endif
	printf(bufferText);
	posa = 0;
#ifndef TERMINAL
	SDL_Delay(8);
#endif
	printf("\x1b[0;0H");
	memset(bufferText, '\0', sizeof bufferText);
}

int main(int argc, char **argv) {
#ifndef TERMINAL
	SDL_Window *window;
#endif
	printf("\e[1;1H\e[2J");
	nes.load(argv[1]);
	nes.reset();
	nes.mem()->set_pressed_buttons(false, KEYS_p1);
	nes.mem()->set_pressed_buttons(true, KEYS_p2);

#ifndef TERMINAL
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("SDL_CreateTexture", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 6, 6, SDL_WINDOW_RESIZABLE);
#endif
	nes.set_render(&draw_pixel_console, &frame_done_sdl);
	while (!stop) {
		nes.cycle();
	}
#ifndef TERMINAL
	SDL_DestroyWindow(window);
	SDL_Quit();
#endif
	return EXIT_SUCCESS;
}
