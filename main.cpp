#include "nes.h"
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <termios.h>
#include <SDL2/SDL.h>
#include <sys/ioctl.h>

NES nes= NES();

uint8_t rowBuffer[400];
char bufferText[256*40*256*2];
unsigned int posa = 0;
bool stop = false;
bool frame = true;

int keys_p1[8] = { 0 };
int keys_p2[8] = { 0 };
uint8_t palette_conv_rgb[] = {
		0x66, 0x66, 0x66, 0x00, 0x2A, 0x88, 0x14, 0x12, 0xA7, 0x3B, 0x00, 0xA4, 0x5C, 0x00, 0x7E, 0x6E, 0x00, 0x40, 0x6C, 0x06, 0x00, 0x56, 0x1D, 0x00,
		0x33, 0x35, 0x00, 0x0B, 0x48, 0x00, 0x00, 0x52, 0x00, 0x00, 0x4F, 0x08, 0x00, 0x40, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xAD, 0xAD, 0xAD, 0x15, 0x5F, 0xD9, 0x42, 0x40, 0xFF, 0x75, 0x27, 0xFE, 0xA0, 0x1A, 0xCC, 0xB7, 0x1E, 0x7B, 0xB5, 0x31, 0x20, 0x99, 0x4E, 0x00,
		0x6B, 0x6D, 0x00, 0x38, 0x87, 0x00, 0x0C, 0x93, 0x00, 0x00, 0x8F, 0x32, 0x00, 0x7C, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFE, 0xFF, 0x64, 0xB0, 0xFF, 0x92, 0x90, 0xFF, 0xC6, 0x76, 0xFF, 0xF3, 0x6A, 0xFF, 0xFE, 0x6E, 0xCC, 0xFE, 0x81, 0x70, 0xEA, 0x9E, 0x22,
		0xBC, 0xBE, 0x00, 0x88, 0xD8, 0x00, 0x5C, 0xE4, 0x30, 0x45, 0xE0, 0x82, 0x48, 0xCD, 0xDE, 0x4F, 0x4F, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFE, 0xFF, 0xC0, 0xDF, 0xFF, 0xD3, 0xD2, 0xFF, 0xE8, 0xC8, 0xFF, 0xFB, 0xC2, 0xFF, 0xFE, 0xC4, 0xEA, 0xFE, 0xCC, 0xC5, 0xF7, 0xD8, 0xA5,
		0xE4, 0xE5, 0x94, 0xCF, 0xEF, 0x96, 0xBD, 0xF4, 0xAB, 0xB3, 0xF3, 0xCC, 0xB5, 0xEB, 0xF2, 0xB8, 0xB8, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t palette_conv_256[] = {
	102, 12, 19, 55, 90, 124, 124, 88, 58, 28, 22, 22, 23, 16, 16, 16, 145, 33, 27, 63, 164, 161, 202, 166, 136, 34, 34, 35, 30, 16, 16, 16, 231, 75,
	69, 105, 213, 204, 209, 215, 214, 154, 77, 84, 44, 102, 16, 16, 231, 153, 147, 183, 219, 217, 223, 223, 222, 192, 157, 158, 14, 225, 16, 16};

void enable_raw_mode()
{
    termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &term);
}

void disable_raw_mode()
{
    termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
}

int pending_events()
{
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting;
}

void draw_pixel_console(int x, int y, uint8_t color) {
	if ((y % 4) > 1)
		return;

	if (y % 2 && y < 240 && y > 2) {
		if (x == 0) {
			posa += snprintf(bufferText + posa, 256, "\x1b[1B\x1b[500D");
		}

		posa += snprintf(bufferText + posa, 256, "\x1b[38;2;%d;%d;%dm", palette_conv_rgb[color*3 + 0],palette_conv_rgb[color*3 + 1], palette_conv_rgb[color*3 + 2]);

		if (rowBuffer[x] != color)
			posa += snprintf(bufferText + posa, 256, "▀");
		else
			posa += snprintf(bufferText + posa, 256, "█");
	} else {
		rowBuffer[x] = color;
	}

}
void frame_done_console() {
	
	frame = !frame;
	printf(bufferText);
	posa = 0;
	printf("\x1b[0;0H");
	memset(bufferText, '\0', sizeof bufferText);
	
	for (int i = 7; i >= 0; i--)
		if (keys_p1[i] > 0) keys_p1[i]--;
	
	for (int i = pending_events(); i > 0; i--) {
		char character = 0;
		read(0, &character, 1);

		if (character == 'w')
			keys_p1[4] = 4;
		else if (character == 'a')
			keys_p1[6] = 4;
		else if (character == 's')
			keys_p1[5] = 4;
		else if (character == 'd')
			keys_p1[7] = 4;
		else if (character == 'e')
			keys_p1[0] = 4;
		else if (character == 'q')
			keys_p1[1] = 4;
		else if (character == 'z')
			keys_p1[2] = 4;
		else if (character == 'x')
			keys_p1[3] = 4;
	}
	nes.mem()->set_pressed_buttons(false, keys_p1);
}

int main(int argc, char **argv) {

	printf("\e[1;1H\e[2J");
	nes.load(argv[1]);
	nes.reset();

	nes.mem()->set_pressed_buttons(false, keys_p1);
	nes.mem()->set_pressed_buttons(true, keys_p2);

	nes.set_render(&draw_pixel_console, &frame_done_console);
	enable_raw_mode();
	while (!stop) {
		nes.cycle();
	}
	disable_raw_mode();
	tcflush(0, TCIFLUSH);
	return 0;
}
