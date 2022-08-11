#include "kernel.h"

// TODO : This is a temporary file. The functions from this file need to be shifted to drivers/terminal after some higher level
// functions are established

// This file provides the lowest level of terminal output functionality

// Before all terminal operations take place, i.e. before writing to the VGA video memory
// at 0xb8000 or manipulating the frame buffer, always make sure that video mode is VGA 80x25 16 bit colour mode
// This is done by calling IsTerminalMode()

static bool TerminalModeVGA80_25 = true;

// Our terminal is always in VGA 80x25 16 bit colour mode
// This mode is initialized by the LOADER32
static char *const TerminalVideoMemory = (char *)0xb8000;
static const size_t VGAWidth = 80, VGAHeight = 25;
static size_t TerminalCursorX = 0, TerminalCursorY = 0;
static const uint8_t DEFAULT_TERMINAL_COLOUR = 0x0f;
static uint8_t CurrentTextColour = 0x0f;
static uint8_t CurrentTerminalColour = 0x0f;
static uint16_t CursorPort = 0x3d4;
static uint16_t CursorPortIndex = 0x3d5;

const char* const HexPalette = "0123456789ABCDEF";

// Check if video mode is 80x25 VGA
bool IsTerminalMode() {
	// TODO : right now we are just checking a boolean value, add functionality to actually check if the mode is the assumed one
	return TerminalModeVGA80_25;
}

// Clears the terminal screen
void TerminalClearScreen() {
	if (!IsTerminalMode()) {
		return;
	}
	// Inline assembly to set 4000 bytes
	// of framebuffer at 0xb8000 to white text on black screen
	TerminalClearScreenASM();
	TerminalCursorX = TerminalCursorY = 0;
	TerminalSetCursorPosition(0, 0);
}

// Set the cursor to a given (x, y) where 0<=x<=79 and 0<=y<=24
void TerminalSetCursorPosition(size_t x, size_t y) {
	if (!IsTerminalMode() || x >= VGAWidth || y >= VGAHeight) {
		return;
	}
	size_t position = y * VGAWidth + x; // Multiply 80 to row and add column to it
	TerminalCursorX = x;
	TerminalCursorY = y;
	OutputByte(CursorPort, 0x0f); // Cursor LOW port to VGA INDEX register
	OutputByte(CursorPortIndex, (uint8_t)(position & 0xff));
	OutputByte(CursorPort, 0x0e); // Cursor HIGH port to VGA INDEX register
	OutputByte(CursorPortIndex, (uint8_t)((position >> 8) & 0xff));
}

void TerminalPutChar(char c) {
	// TODO: Add text scroll. Newline behaviour needs to be better.
	if (!IsTerminalMode() || TerminalCursorX >= VGAWidth || TerminalCursorY >= VGAHeight) {
		TerminalCursorX = TerminalCursorY = 0;
		return;
	}
	if (c == '\n') {
		goto skip;
	}
	const size_t index = (TerminalCursorY * VGAWidth + TerminalCursorX) * 2;
	TerminalVideoMemory[index] = c;
	TerminalVideoMemory[index + 1] = CurrentTextColour;
	if (++TerminalCursorX == VGAWidth) {
	skip:
		TerminalCursorX = 0;
		if (++TerminalCursorY == VGAHeight) {
			TerminalCursorY = 0;
			TerminalClearScreen();
		}
	}
	return;
}

// Prints a string of given length to the terminal
void TerminalPrintString(const char *str, const size_t length) {
	// We are printing only length number of characters to
	// the terminal to avoid buffer overrun which may be caused by no null char at end of string
	if (!IsTerminalMode()) {
		return;
	}
	for (size_t i = 0; i < length; ++i, ++str) {
		TerminalPutChar(*str);
	}
	TerminalSetCursorPosition(TerminalCursorX, TerminalCursorY);
}

void TerminalPrintHex(void* value, size_t size) {
	TerminalPrintString("[0x", 3);
	char hexValue[size * 2];
	for (size_t i = 0; i < size; ++i) {
		uint8_t x = *((uint8_t*)value + i);
		hexValue[size * 2 - 1 - i * 2] = HexPalette[x & 0xf];
		hexValue[size * 2 - 2 - i * 2] = HexPalette[(x >> 4) & 0xf];
	}
	TerminalPrintString(hexValue, size * 2);
	TerminalPutChar(']');
	TerminalSetCursorPosition(TerminalCursorX, TerminalCursorY);
}