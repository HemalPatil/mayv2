#include <apic.h>
#include <drivers/storage/ahci.h>
#include <elf64.h>
#include <heapmemmgmt.h>
#include <idt64.h>
#include <interrupts.h>
#include <kernel.h>
#include <pcie.h>
#include <phymemmgmt.h>
#include <sse2.h>
#include <string.h>
#include <terminal.h>
#include <tss64.h>
#include <vbe.h>
#include <virtualmemmgmt.h>

static const char* const kernelLoadedStr = "Kernel loaded\nRunning in 64-bit long mode\n\n";
static const char* const kernelPanicStr = "\n!!! Kernel panic !!!\n!!! Halting the system !!!\n";

InfoTable *infoTable;

// First C-function to be called
void kernelMain(
	InfoTable *infoTableAddress,
	size_t lowerHalfSize,
	size_t higherHalfSize,
	void* usablePhyMemStart
) {
	// Be careful of NULL references until the IVTs in
	// identity mapped page 0 of virtual address space
	// are moved somewhere else during interrupt initialization
	infoTable = infoTableAddress;

	terminalSetBgColour(TERMINAL_COLOUR_BLUE);
	terminalSetTextColour(TERMINAL_COLOUR_BWHITE);
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernelLoadedStr, strlen(kernelLoadedStr));

	// Initialize physical memory
	size_t phyMemBuddyPagesCount;
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
	if (!setupSse2()) {
		kernelPanic();
	}
	terminalPrintChar('\n');

	if (!parseAcpi3()) {
		kernelPanic();
	}

	// Disable PIC and setup APIC
	if (!initializeApic()) {
		kernelPanic();
	}

	// Enumerate PCIe devices
	if (!enumeratePCIe()) {
		kernelPanic();
	}

	// Start drivers for PCIe devices
	// TODO: the if-else will become very complicated
	PCIeFunction *pcieFunction = pcieFunctions;
	while (pcieFunction) {
		bool (*initializer)(PCIeFunction *pcieFunction) = INVALID_ADDRESS;
		if (
			pcieFunction->configurationSpace->class == PCI_CLASS_STORAGE &&
			pcieFunction->configurationSpace->subClass == PCI_SUBCLASS_SATA &&
			pcieFunction->configurationSpace->progIf == PCI_PROG_AHCI
		) {
			initializer = &initializeAHCI;
		}
		if (initializer != INVALID_ADDRESS && !((*initializer)(pcieFunction))) {
			kernelPanic();
		}
		pcieFunction = pcieFunction->next;
	}

	// Set up graphical video mode
	// if (!setupGraphicalVideoMode()) {
	// 	kernelPanic();
	// }

	// Setup basic hardware interrupts
	// if (!initializeInterrupts()) {
	// 	kernelPanic();
	// }
}

void kernelPanic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicStr, strlen(kernelPanicStr));
	hangSystem(true);
}
