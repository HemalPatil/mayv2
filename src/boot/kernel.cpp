#include <acpi.h>
#include <apic.h>
#include <cstring>
#include <drivers/filesystems/iso9660.h>
#include <drivers/ps2/keyboard.h>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <drivers/timers/hpet.h>
#include <heapmemmgmt.h>
#include <idt64.h>
#include <kernel.h>
#include <pcie.h>
#include <phymemmgmt.h>
#include <sse4.h>
#include <terminal.h>
#include <tss64.h>
// #include <vbe.h>
#include <virtualmemmgmt.h>

#include <future>

static const char* const kernelLoadedStr = "Kernel loaded\nRunning in 64-bit long mode\n\n";
static const char* const kernelPanicStr = "\n!!! Kernel panic !!!\n!!! Halting the system !!!\n";

InfoTable *infoTable;

extern "C" {

void kernelMain(
	InfoTable *infoTableAddress,
	size_t lowerHalfSize,
	size_t higherHalfSize,
	void* usablePhyMemStart
) {
	// Be careful of nullptr references until the IVTs in
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
	if (!enableSse4()) {
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

	// Get at least 1 periodic 64-bit edge-triggered HPET
	if (!initializeHpet()) {
		kernelPanic();
	}

	// Setup basic hardware interrupts
	if (!initializePs2Keyboard()) {
		kernelPanic();
	}
	enableInterrupts();
	terminalPrintChar('\n');

	// Enumerate PCIe devices
	if (!enumeratePCIe()) {
		kernelPanic();
	}

	// Start drivers for PCIe devices
	PCIeFunction *pcieFunction = pcieFunctions;
	while (pcieFunction) {
		bool (*initializer)(PCIeFunction *pcieFunction) = nullptr;
		if (
			pcieFunction->configurationSpace->mainClass == PCI_CLASS_STORAGE &&
			pcieFunction->configurationSpace->subClass == PCI_SUBCLASS_SATA &&
			pcieFunction->configurationSpace->progIf == PCI_PROG_AHCI
		) {
			initializer = &AHCI::initialize;
		}
		if (initializer && !((*initializer)(pcieFunction))) {
			kernelPanic();
		}
		pcieFunction = pcieFunction->next;
	}

	// Create filesystems
	AHCI::Controller *c = AHCI::controllers;
	while (c) {
		for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
			AHCI::Device *device = c->getDevice(i);
			if (device) {
				// Try with ISO9660 for SATAPI devices first because that is the most likely FS
				if (AHCI::Device::Type::Satapi == device->getType()) {
					if (FS::ISO9660::isIso9660(device)) {

					}
				}
			}
		}
		c = c->next;
	}

	// Set up graphical video mode
	// if (!setupGraphicalVideoMode()) {
	// 	kernelPanic();
	// }
	hangSystem(false);
}

void kernelPanic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicStr, strlen(kernelPanicStr));
	hangSystem(true);
}

}
