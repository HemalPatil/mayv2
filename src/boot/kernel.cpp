#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/filesystems/iso9660.h>
#include <drivers/ps2/keyboard.h>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <drivers/timers/hpet.h>
#include <idt64.h>
#include <kernel.h>
#include <pcie.h>
#include <sse4.h>
#include <terminal.h>
#include <tss64.h>
// #include <vbe.h>

static const char* const kernelLoadedStr = "Kernel loaded\nRunning in 64-bit long mode\n\n";
static const char* const kernelPanicStr = "\n!!! Kernel panic !!!\n!!! Halting the system !!!\n";
static const char* const creatingFsStr = "Creating file systems";
static const char* const creatingFsDoneStr = "File systems created\n";
static const char* const checkingAhciStr = "Checking AHCI controllers";
static const char* const checkingAhciDoneStr = "AHCI controllers checked\n";
static const char* const isoFoundStr = "ISO filesystem found at ";
static const char* const globalCtorStr = "Running global constructors";

InfoTable *Kernel::infoTable;

extern "C" {

[[noreturn]] void kernelMain(
	InfoTable *infoTableAddress,
	size_t lowerHalfSize,
	size_t higherHalfSize,
	void* usablePhyMemStart
) {
	// Be careful of nullptr references until the IVTs in
	// identity mapped page 0 of virtual address space
	// are moved somewhere else during interrupt initialization
	Kernel::infoTable = infoTableAddress;
	Kernel::GlobalConstructor globalCtors[Kernel::infoTable->globalCtorsCount];
	memcpy(
		globalCtors,
		(void*)Kernel::infoTable->globalCtorsLocation,
		Kernel::infoTable->globalCtorsCount * sizeof(uint64_t)
	);

	terminalSetBgColour(TERMINAL_COLOUR_BLUE);
	terminalSetTextColour(TERMINAL_COLOUR_BWHITE);
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernelLoadedStr, strlen(kernelLoadedStr));

	// Initialize physical memory
	size_t phyMemBuddyPagesCount;
	if (!Kernel::Memory::Physical::initialize(
		usablePhyMemStart,
		lowerHalfSize,
		higherHalfSize,
		phyMemBuddyPagesCount
	)) {
		Kernel::panic();
	}

	// Initialize virtual memory
	if (!Kernel::Memory::Virtual::initialize(
		(void*)(KERNEL_HIGHERHALF_ORIGIN + higherHalfSize),
		lowerHalfSize,
		phyMemBuddyPagesCount,
		globalCtors
	)) {
		Kernel::panic();
	}

	// Initialize TSS first because ISTs in IDT require TSS
	setupTss64();
	setupIdt64();
	if (!enableSse4()) {
		Kernel::panic();
	}
	terminalPrintChar('\n');

	if (!parseAcpi3()) {
		Kernel::panic();
	}

	// Disable PIC and setup APIC
	if (!initializeApic()) {
		Kernel::panic();
	}

	// Get at least 1 periodic 64-bit edge-triggered HPET
	if (!initializeHpet()) {
		Kernel::panic();
	}

	// Setup basic hardware interrupts
	if (!initializePs2Keyboard()) {
		Kernel::panic();
	}

	// Enable interrupts
	terminalPrintChar('\n');
	enableInterrupts();
	terminalPrintChar('\n');

	// Enumerate PCIe devices
	if (!PCIe::enumerate()) {
		Kernel::panic();
	}

	// Start drivers for PCIe devices
	for (auto &function : PCIe::functions) {
		bool (*initializer)(PCIe::Function &pcieFunction) = nullptr;
		if (
			function.configurationSpace->mainClass == PCIe::Class::Storage &&
			function.configurationSpace->subClass == PCIe::Subclass::Sata &&
			function.configurationSpace->progIf == PCIe::ProgramType::Ahci
		) {
			initializer = &AHCI::initialize;
		}
		if (initializer && !((*initializer)(function))) {
			Kernel::panic();
		}
	}

	// Create filesystems
	terminalPrintString(creatingFsStr, strlen(creatingFsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(checkingAhciStr, strlen(checkingAhciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	for (size_t controllerCount = 0; auto &controller : AHCI::controllers) {
		for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
			AHCI::Device *device = controller.getDevice(i);
			if (device) {
				// Try with ISO9660 for SATAPI devices first because that is the most likely FS
				if (AHCI::Device::Type::Satapi == device->getType()) {
					if (FS::ISO9660::isIso9660(device)) {
						terminalPrintSpaces4();
						terminalPrintSpaces4();
						terminalPrintString(isoFoundStr, strlen(isoFoundStr));
						terminalPrintDecimal(controllerCount);
						terminalPrintChar(':');
						terminalPrintDecimal(i);
						terminalPrintChar('\n');
						FS::filesystems.push_back(std::make_shared<FS::ISO9660>(*device));
					}
				}
			}
		}
		++controllerCount;
	}
	terminalPrintSpaces4();
	terminalPrintString(checkingAhciDoneStr, strlen(checkingAhciDoneStr));
	terminalPrintString(creatingFsDoneStr, strlen(creatingFsDoneStr));

	// Set up graphical video mode
	// if (!setupGraphicalVideoMode()) {
	// 	panic();
	// }
	Kernel::hangSystem();
}

void panic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicStr, strlen(kernelPanicStr));
	Kernel::hangSystem();
}

}
