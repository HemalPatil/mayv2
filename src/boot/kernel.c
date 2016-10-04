#include "kernel.h"

// First C-function to be called
void KernelMain(uint16_t* InfoTableAddress)
{
	InfoTable = InfoTableAddress;

	//TerminalClearScreen();
	//TerminalSetCursor(0, 0);

	uint64_t mem = GetKernelSize();
}