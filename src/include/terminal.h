#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEFAULT_TERMINAL_COLOUR 0x0f

#define TERMINAL_COLOUR_BLACK		0x0
#define TERMINAL_COLOUR_BLUE		0x1
#define TERMINAL_COLOUR_GREEN		0x2
#define TERMINAL_COLOUR_CYAN		0x3
#define TERMINAL_COLOUR_RED			0x4
#define TERMINAL_COLOUR_MAGENTA		0x5
#define TERMINAL_COLOUR_BROWN		0x6
#define TERMINAL_COLOUR_WHITE		0x7
#define TERMINAL_COLOUR_GREY		0x8
#define TERMINAL_COLOUR_LBLUE		0x9
#define TERMINAL_COLOUR_LGREEN		0xa
#define TERMINAL_COLOUR_LCYAN		0xb
#define TERMINAL_COLOUR_LRED		0xc
#define TERMINAL_COLOUR_LMAGENTA	0xd
#define TERMINAL_COLOUR_YELLOW		0xe
#define TERMINAL_COLOUR_BWHITE		0xf

extern bool isTerminalMode();
extern void terminalClearLine(size_t lineNumber);
extern void terminalClearScreen();
extern void terminalGetCursorPosition(size_t *x, size_t *y);
extern void terminalPrintChar(char);
extern void terminalPrintDecimal(int64_t value);
extern void terminalPrintHex(void* value, size_t size);
extern void terminalPrintSpaces4();
extern void terminalPrintString(const char* const str, const size_t length);
extern void terminalScroll(size_t lineCount);
extern void terminalSetBgColour(uint8_t colour);
extern void terminalSetCursorPosition(size_t x, size_t y);
extern void terminalSetTextColour(uint8_t colour);