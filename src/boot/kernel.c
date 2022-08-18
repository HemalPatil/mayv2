#include <string.h>
#include "kernel.h"

const char* const infoTableStr = "InfoTable located at ";
const char* const kernel64Loaded = "Kernel64 loaded\nRunning in 64-bit long mode\n";
const char* const systemInitializationFailed = "\nSystem initialization failed. Cannot boot. Halting the system\n";
const char* const kernelPanicString = "\nKernel panic!!!\n";

InfoTable *infoTable;

// First C-function to be called
void kernelMain(InfoTable *infoTableAddress) {
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernel64Loaded, strlen(kernel64Loaded));

	// InfoTable is our custom structure which has some essential information about the system
	// gained during system boot; and the information that is best found out about in real mode.
	infoTable = infoTableAddress;
	terminalPrintString(infoTableStr, strlen(infoTableStr));
	terminalPrintHex(&infoTable, sizeof(infoTable));
	terminalPrintChar('\n');

	// Do memory setup
	if (!initializePhysicalMemory()) {
		kernelPanic(systemInitializationFailed);
	}
	return;
	// if (!InitializeVirtualMemory(SystemInitializationFailed))
	// {
	// 	KernelPanic();
	// }

	// Initialize TSS first because ISTs in IDT require TSS
	setupTss64();
	setupIdt64();
	
	if (!parseAcpi3()) {
		kernelPanic(systemInitializationFailed);
	}

	// Disable PIC and setup APIC
	setupApic();

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

void kernelPanic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicString, strlen(kernelPanicString));
	hangSystem();
}