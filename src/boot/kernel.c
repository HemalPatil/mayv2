#include "kernel.h"
#include<string.h>

const char* HelloWorld = "Hello World!";
const char* SystemInitializationFailed = "System initialization failed. Cannot boot. Halting the system";

// First C-function to be called
void KernelMain(uint16_t* InfoTableAddress)
{
	// InfoTable is our custom structure which has some essential information about the system
	// gained during system boot; and the information that is best found out about in real mode.
	InfoTable = InfoTableAddress;

	// Do memory setup
	if (!InitializePhysicalMemory())
	{
		KernelPanic(SystemInitializationFailed);
	}
	if (!InitializeVirtualMemory(SystemInitializationFailed))
	{
		KernelPanic();
	}

	// Get basic interrupts and exceptions setup
	// Initializing ACPI will generate page faults while parsing ACPI tables
	// so we must first fill the IDT with offsets to basic exceptions and interrupts
	PopulateIDTWithOffsets();
	if(!InitializeACPI3())
	{
		KernelPanic(SystemInitializationFailed);
	}
	if(!SetupHardwareInterrupts())
	{
		KernelPanic(SystemInitializationFailed);
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