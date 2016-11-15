#include "kernel.h"
#include<string.h>

const char* HelloWorld = "Hello World!";
const char* SystemInitializationFailed = "System initialization failed. Cannot boot. Halting the system";
const char* str1 = "hello";
const char* str2 = "Helwo";

// First C-function to be called
void KernelMain(uint16_t* InfoTableAddress)
{
	// InfoTable is our custom structure which has some essential information about the system
	// gained during system boot; and the information that is best found out about in real mode.
	InfoTable = InfoTableAddress;

	if(!InitializeACPI3())
	{
		KernelPanic(SystemInitializationFailed);
	}

	if(!SetupInterrupts())
	{
		KernelPanic(SystemInitializationFailed);
	}

	if (!InitializePhysicalMemory())
	{
		KernelPanic(SystemInitializationFailed);
	}

	if (!InitializeVirtualMemory(SystemInitializationFailed))
	{
		KernelPanic();
	}

	TerminalSetCursorPosition(33,12);
	TerminalPrintString(HelloWorld, strlen(HelloWorld));
}

void KernelPanic(const char* ErrorString)
{
	// TODO : add kernel panic implementation
	TerminalClearScreen();
	TerminalSetCursorPosition(0, 0);
	TerminalPrintString(ErrorString, strlen(ErrorString));
	HangSystem();
}