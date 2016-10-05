#include "kernel.h"

// First C-function to be called
void KernelMain(uint16_t* InfoTableAddress)
{
	InfoTable = InfoTableAddress;

	//OutputByte(0x160, 0xca);

	//TerminalClearScreen();
	TerminalSetCursor(79, 0);
}