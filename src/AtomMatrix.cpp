//
// AtomMatrix.cpp
//

#include <M5Atom.h>

#include "AtomMatrix.h"

static const uint8_t ledCharData[LED_CHAR_COUNT][5] = {
	{ 0b01100, 0b10010, 0b10010, 0b10010, 0b01100, },  // "0"
	{ 0b00100, 0b01100, 0b00100, 0b00100, 0b01110, },  // "1"
	{ 0b01100, 0b10010, 0b00100, 0b01000, 0b11110, },  // "2"
	{ 0b11100, 0b00010, 0b01100, 0b00010, 0b11100, },  // "3"
	{ 0b00100, 0b01100, 0b10100, 0b11110, 0b00100, },  // "4"
	{ 0b11110, 0b10000, 0b11100, 0b00010, 0b11100, },  // "5"
	{ 0b01100, 0b10000, 0b11100, 0b10010, 0b01100, },  // "6"
	{ 0b11110, 0b00010, 0b00100, 0b01000, 0b01000, },  // "7"
	{ 0b01100, 0b10010, 0b01100, 0b10010, 0b01100, },  // "8"
	{ 0b01100, 0b10010, 0b01110, 0b00010, 0b01100, },  // "9"
	{ 0b01100, 0b10010, 0b11110, 0b10010, 0b10010, },  // "A"
	{ 0b11100, 0b10010, 0b11100, 0b10010, 0b11100, },  // "B"
	{ 0b01100, 0b10010, 0b10000, 0b10010, 0b01100, },  // "C"
	{ 0b11100, 0b10010, 0b10010, 0b10010, 0b11100, },  // "D"
	{ 0b11110, 0b10000, 0b11100, 0b10000, 0b11110, },  // "E"
	{ 0b11110, 0b10000, 0b11100, 0b10000, 0b10000, },  // "F"
	{ 0b00100, 0b00100, 0b00100, 0b00000, 0b00100, },  // "!"
	{ 0b01110, 0b10001, 0b00110, 0b00000, 0b00100, },  // "?"
	{ 0b00000, 0b00000, 0b10101, 0b00000, 0b00000, },  // "..."
	{ 0b00100, 0b01000, 0b11111, 0b01000, 0b00100, },  // "<-"
	{ 0b00100, 0b00010, 0b11111, 0b00010, 0b00100, },  // "->"
	{ 0b00100, 0b01110, 0b10101, 0b00100, 0b00100, },  // "up"
	{ 0b00100, 0b00100, 0b10101, 0b01110, 0b00100, },  // "down"
	{ 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, },  // "turn left"
	{ 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, },  // "turn right"
	{ 0b00000, 0b00100, 0b01010, 0b10001, 0b00000, },  // "forward"
	{ 0b00000, 0b10001, 0b01010, 0b00100, 0b00000, },  // "backward"
	{ 0b00000, 0b00100, 0b01010, 0b00100, 0b00000, },  // "OK(S)"
	{ 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, },  // "OK(L)"
	{ 0b00000, 0b01010, 0b00100, 0b01010, 0b00000, },  // "NG(S)"
	{ 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, },  // "NG(L)"
	{ 0b01010, 0b10001, 0b00000, 0b10001, 0b01010, },  // "scan/rotate"
	{ 0b01100, 0b00001, 0b10001, 0b10000, 0b00110, },  // "rotate Left"
	{ 0b00110, 0b10000, 0b10001, 0b00001, 0b01100, },  // "rotate right"
	{ 0b00000, 0b01010, 0b01010, 0b01010, 0b00000, },  // "pause"
};

void drawLEDChar(int index, uint32_t rgb, int shiftX, int shiftY) {
	static const int LED_SIZE = 5;
	uint8_t buff[LED_SIZE * LED_SIZE * 3 + 2];

	buff[0] = LED_SIZE;
	buff[1] = LED_SIZE;

	for (int i = 0; i < (LED_SIZE * LED_SIZE); i++) {
		int x = ((i + shiftX) % LED_SIZE);
		if (x < 0) {
			x += LED_SIZE;
		}
		int y = ((i / LED_SIZE) + shiftY) % LED_SIZE;
		if (y < 0) {
			y += LED_SIZE;
		}
		for (int j = 0; j < 3; j++) {
			bool bit = (ledCharData[index][y] & (1 << (LED_SIZE - 1 - x))) != 0;
			int shift = (j == 0) ? 8 : (j == 1) ? 16 : 0;
			buff[i * 3 + j + 2] = bit ? ((rgb >> shift) & 0xFF) : 0x00;
		}
	}
	M5.dis.displaybuff(buff, 0, 0);
}
