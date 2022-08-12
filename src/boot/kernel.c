#include <string.h>
#include "kernel.h"

const char* const Kernel64Loaded = "Kernel64 loaded\nRunning in 64-bit long mode\n";
const char* const SystemInitializationFailed = "\nSystem initialization failed. Cannot boot. Halting the system\n";
const char* const KernelPanicString = "\nKernel panic!!!\n";

uint16_t *InfoTable;

// First C-function to be called
void KernelMain(uint16_t *InfoTableAddress) {
	// InfoTable is our custom structure which has some essential information about the system
	// gained during system boot; and the information that is best found out about in real mode.
	InfoTable = InfoTableAddress;

	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(Kernel64Loaded, strlen(Kernel64Loaded));

	// Do memory setup
	if (!InitializePhysicalMemory()) {
		KernelPanic(SystemInitializationFailed);
	}
	return;
	// if (!InitializeVirtualMemory(SystemInitializationFailed))
	// {
	// 	KernelPanic();
	// }

	// Initialize TSS first because ISTs in IDT require TSS
	SetupTSS64();
	SetupIDT64();
	
	if (!ParseACPI3()) {
		KernelPanic(SystemInitializationFailed);
	}

	// Disable PIC and setup APIC
	SetupAPIC();

	// Get basic interrupts and exceptions setup
	// Initializing ACPI will generate page faults while parsing ACPI tables
	// so we must first fill the IDT with offsets to basic exceptions and interrupts
	// PopulateIDT();
	// if (!SetupHardwareInterrupts())
	// {
	// 	KernelPanic(SystemInitializationFailed);
	// }

	// terminalSetCursorPosition(33, 12);
	// terminalPrintString(HelloWorld, strlen(HelloWorld));
}

void KernelPanic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(KernelPanicString, strlen(KernelPanicString));
	HangSystem();
}