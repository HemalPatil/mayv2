#include <string.h>
#include "kernel.h"

const char *Kernel64Loaded = "Kernel64 loaded\nRunning in 64-bit long mode\n";
const char *SystemInitializationFailed = "\nSystem initialization failed. Cannot boot. Halting the system\n";

uint16_t *InfoTable;

// First C-function to be called
void KernelMain(uint16_t *InfoTableAddress)
{
	// TerminalSetCursorPosition(0, 0);
	// TerminalPrintString(HelloWorld, strlen(HelloWorld));
	TerminalClearScreen();
	TerminalSetCursorPosition(0, 0);
	TerminalPrintString(Kernel64Loaded, strlen(Kernel64Loaded));


	// Initialize TSS first because ISTs in IDT require TSS
	SetupTSS64();
	SetupIDT64();

	// InfoTable is our custom structure which has some essential information about the system
	// gained during system boot; and the information that is best found out about in real mode.
	InfoTable = InfoTableAddress;

	// Do memory setup
	// if (!InitializePhysicalMemory())
	// {
	// 	KernelPanic(SystemInitializationFailed);
	// }
	// if (!InitializeVirtualMemory(SystemInitializationFailed))
	// {
	// 	KernelPanic();
	// }

	// Get basic interrupts and exceptions setup
	// Initializing ACPI will generate page faults while parsing ACPI tables
	// so we must first fill the IDT with offsets to basic exceptions and interrupts
	// PopulateIDT();
	// if (!InitializeACPI3())
	// {
	// 	KernelPanic(SystemInitializationFailed);
	// }
	// if (!SetupHardwareInterrupts())
	// {
	// 	KernelPanic(SystemInitializationFailed);
	// }

	// TerminalSetCursorPosition(33, 12);
	// TerminalPrintString(HelloWorld, strlen(HelloWorld));
}

void KernelPanic(const char *ErrorString)
{
	// TODO : add kernel panic implementation
	TerminalPrintString(ErrorString, strlen(ErrorString));
	HangSystem();
}