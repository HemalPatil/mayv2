#include "kernel.h"

// First C-function to be called
void KernelMain(uint16_t* InfoTableAddress)
{
	// InfoTable is our custom structure which has some essential information about the system
	// gained during system boot; and the information that is best found out about in real mode.
	InfoTable = InfoTableAddress;

	if (!InitializePhysicalMemory())
	{
		KernelPanic();
	}

	if (!InitializeVirtualMemory())
	{
		KernelPanic();
	}

	TerminalSetCursorPosition(0,1);
}

void KernelPanic()
{
	// TODO : add kernel panic implementation
	HaltSystem();
}