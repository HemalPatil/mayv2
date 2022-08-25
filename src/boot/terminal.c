#include <io.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>

// TODO : This is a temporary file. The functions from this file need to be shifted to drivers/terminal after some higher level
// functions are established

// This file provides the lowest level of terminal output functionality

// Before all terminal operations take place, i.e. before writing to the VGA video memory
// at 0xb8000 or manipulating the frame buffer, always make sure that video mode is VGA 80x25 16 bit colour mode
// This is done by calling isTerminalMode()

static bool vgaMode80x25 = true;

// Our terminal is always in VGA 80x25 16 bit colour mode
// This mode is initialized by the LOADER32
static char *const videoMemory = (char *)0xb8000;
static const size_t vgaWidth = 80;
static const size_t vgaHeight = 25;
static const size_t videoMemSize = vgaWidth * vgaHeight * 2;
static size_t cursorX = 0, cursorY = 0;
static uint8_t currentTextColour = DEFAULT_TERMINAL_COLOUR & 0xf;
static uint8_t currentBgColour = DEFAULT_TERMINAL_COLOUR >> 4;
static uint8_t currentTerminalColour = DEFAULT_TERMINAL_COLOUR;
static uint16_t cursorPort = 0x3d4;
static uint16_t cursorPortIndex = 0x3d5;

const char* const hexPalette = "0123456789ABCDEF";
const char* const spaces4 = "    ";

// Check if video mode is 80x25 VGA
bool isTerminalMode() {
	// FIXME: Add functionality to actually check if the mode is the assumed one
	return vgaMode80x25;
}

void terminalSetBgColour(uint8_t colour) {
	currentBgColour = colour;
	currentTerminalColour = currentBgColour << 4 | currentTextColour;
}

void terminalSetTextColour(uint8_t colour) {
	currentTextColour = colour;
	currentTerminalColour = currentBgColour << 4 | currentTextColour;
}

// Clears the terminal screen
void terminalClearScreen() {
	if (!isTerminalMode()) {
		return;
	}
	for (size_t i = 0; i < vgaHeight; ++i) {
		terminalClearLine(i);
	}
	cursorX = cursorY = 0;
	terminalSetCursorPosition(0, 0);
}

void terminalClearLine(size_t lineNumber) {
	uint64_t data = 0;
	for (size_t i = 0; i < 4; ++i) {
		data <<= 8;
		data |= currentTerminalColour;
		data <<= 8;
		data |= 0x20;
	}
	uint64_t *lineAddress = (uint64_t*)(videoMemory + lineNumber * vgaWidth * 2);
	for (size_t i = 0; i < 20; ++i, ++lineAddress) {
		*lineAddress = data;
	}
}

// Set the cursor to a given (x, y) where 0<=x<=79 and 0<=y<=24
void terminalSetCursorPosition(size_t x, size_t y) {
	if (!isTerminalMode() || x >= vgaWidth || y >= vgaHeight) {
		return;
	}
	cursorX = x;
	cursorY = y;
	size_t position = y * vgaWidth + x; // Multiply 80 to row and add column to it
	outputByte(cursorPort, 0x0f); // Cursor LOW port to VGA INDEX register
	outputByte(cursorPortIndex, (uint8_t)(position & 0xff));
	outputByte(cursorPort, 0x0e); // Cursor HIGH port to VGA INDEX register
	outputByte(cursorPortIndex, (uint8_t)((position >> 8) & 0xff));
}

void terminalScroll(size_t lineCount) {
	// TODO: hide scrolled data somewhere so we can use pgUp and pgDown
	size_t count = lineCount * vgaWidth * 2;
	terminalPrintHex(&count, sizeof(count));
	memcpy(videoMemory, videoMemory + count, videoMemSize - count);
	terminalClearLine(vgaHeight - 1);
}

void terminalPrintChar(char c) {
	// TODO: Add text scroll. Newline behaviour needs to be better.
	if (!isTerminalMode() || cursorX >= vgaWidth || cursorY >= vgaHeight) {
		cursorX = cursorY = 0;
		return;
	}
	if (c == '\n') {
		goto terminalPrintCharUglySkip;
	} else {
		const size_t index = (cursorY * vgaWidth + cursorX) * 2;
		videoMemory[index] = c;
		videoMemory[index + 1] = currentTerminalColour;
		++cursorX;
	}
	if (cursorX >= vgaWidth) {
		terminalPrintCharUglySkip:
			cursorX = 0;
			++cursorY;
	}
	if (cursorY >= vgaHeight) {
		terminalScroll(1);
		cursorX = 0;
		cursorY = vgaHeight - 1;
	}
	terminalSetCursorPosition(cursorX, cursorY);
}

// Prints a string of given length to the terminal
void terminalPrintString(const char *str, const size_t length) {
	// We are printing only length number of characters to
	// the terminal to avoid buffer overrun which may be caused by no null char at end of string
	if (!isTerminalMode()) {
		return;
	}
	for (size_t i = 0; i < length; ++i, ++str) {
		terminalPrintChar(*str);
	}
}

void terminalPrintSpaces4() {
	terminalPrintString(spaces4, 4);
}

void terminalPrintHex(void* value, size_t size) {
	terminalPrintString("[0x", 3);
	char hexValue[size * 2];
	for (size_t i = 0; i < size; ++i) {
		uint8_t x = *((uint8_t*)value + i);
		hexValue[size * 2 - 1 - i * 2] = hexPalette[x & 0xf];
		hexValue[size * 2 - 2 - i * 2] = hexPalette[(x >> 4) & 0xf];
	}
	terminalPrintString(hexValue, size * 2);
	terminalPrintChar(']');
}