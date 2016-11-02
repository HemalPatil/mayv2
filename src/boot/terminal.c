#include "kernel.h"

// TODO : This is a temporary file. The functions from this file need to be shifted to drivers/terminal after some higher level
// functions are established

// This file provides the lowest level of terminal output functionality

// Before all terminal operations take place, i.e. before writing to the VGA video memory
// at 0xb8000 or manipulating the frame buffer, always make sure that video mode is VGA 80x25 16 bit colour mode
// This is done by calling IsTerminalMode()

bool TerminalModeVGA80_25 = true;

// Our terminal is always in VGA 80x25 16 bit colour mode
// This mode is initialized by the LOADER32
char* const TerminalVideoMemory = (char*)0xb8000;
const size_t VGAWidth = 80, VGAHeight = 25;
size_t TerminalCursorX = 0, TerminalCursorY = 0;

// Check if video mode is 80x25 VGA
bool IsTerminalMode()
{
	// TODO : right now we are just checking a boolean value, add functionality to actually check if the mode is the assumed one
	return TerminalModeVGA80_25;
}

// Clears the terminal screen
void TerminalClearScreen()
{
	if (!IsTerminalMode()) { return; }
	// Following code is inline assembly using AT&T syntax for the GCC assemlber
	// set 4000 bytes of framebuffer at 0xb8000 to white text on black screen
	asm("push %rax\n"
		"push %rdi\n"
		"push %rcx\n"
		"mov $0xb8000, %edi\n"
		"mov $0x1f4, %ecx\n"
		"movabs $0x0f200f200f200f20, %rax\n"
		"rep stos %rax, %es:(%rdi)\n"
		"pop %rcx\n"
		"pop %rdi\n"
		"pop %rax");
	TerminalCursorX = TerminalCursorY = 0;
	TerminalSetCursorPosition(0, 0);
}

// Set the cursor to a given x,y point where 0<=x<=79 and 0<=y<=24
void TerminalSetCursorPosition(size_t x, size_t y)
{
	if (!IsTerminalMode() || x>=80 || y>=25)
	{
		return;
	}
	size_t position = y * 80 + x;	// Multiply 80 to row and add column to it
	OutputByte(0x3d4, 0x0f);	// Cursor LOW port to VGA INDEX register
	OutputByte(0x3d5, (uint8_t)(position & 0xff));
	OutputByte(0x3d4, 0x0e);	// Cursor HIGH port to VGA INDEX register
	OutputByte(0x3d5, (uint8_t)((position >> 8) & 0xff));
}