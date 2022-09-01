#include <apic.h>
#include <elf64.h>
#include <heapmemmgmt.h>
#include <idt64.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <tss64.h>
#include <virtualmemmgmt.h>

const char* const infoTableStr = "InfoTable located at ";
const char* const kernelLoadedStr = "Kernel loaded\nRunning in 64-bit long mode\n\n";
const char* const kernelPanicString = "\n!!! Kernel panic !!!\n!!! Halting the system !!!\n";
const char* const lowerHalfStr = "Kernel lower half size ";
const char* const usableMem = "Usable physical memory start ";
const char* const higherHalfStr = "Kernel higher half size ";

InfoTable *infoTable;

// First C-function to be called
void kernelMain(
	InfoTable *infoTableAddress,
	ELF64ProgramHeader* programHeader,
	size_t headerEntryCount,
	void* usablePhyMemStart
) {
	infoTable = infoTableAddress;

	terminalSetBgColour(TERMINAL_COLOUR_BLUE);
	terminalSetTextColour(TERMINAL_COLOUR_BWHITE);
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernelLoadedStr, strlen(kernelLoadedStr));

	// Parse the ELF header and find the size of kernel in lower and higher half
	size_t higherHalfSize = 0;
	size_t lowerHalfSize = 0;
	for (uint16_t i = 0; i < headerEntryCount; ++i) {
		if (programHeader[i].segmentType != ELF_SEGMENT_TYPE_LOAD) {
			continue;
		}
		size_t pageCount = programHeader[i].segmentSizeInMemory / pageSize;
		if (programHeader[i].segmentSizeInMemory != pageCount * pageSize) {
			++pageCount;
		}
		if (programHeader[i].virtualAddress >= KERNEL_HIGHERHALF_ORIGIN) {
			higherHalfSize += pageCount * pageSize;
		} else {
			lowerHalfSize += pageCount * pageSize;
		}
	}

	// Initialize physical memory
	size_t phyMemBuddyPagesCount = 0;
	if (!initializePhysicalMemory(usablePhyMemStart, lowerHalfSize, higherHalfSize, &phyMemBuddyPagesCount)) {
		kernelPanic();
	}

	// Initialize virtual memory
	if (!initializeVirtualMemory((void*)(KERNEL_HIGHERHALF_ORIGIN + higherHalfSize), lowerHalfSize, phyMemBuddyPagesCount)) {
		kernelPanic();
	}

	// Initialize dynamic memory
	if (!initializeDynamicMemory()) {
		kernelPanic();
	}

	// Initialize TSS first because ISTs in IDT require TSS
	setupTss64();
	setupIdt64();
	return;
	
	if (!parseAcpi3()) {
		kernelPanic();
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