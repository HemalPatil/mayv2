#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/filesystems/iso9660.h>
#include <drivers/ps2/keyboard.h>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
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
static const char* const creatingFsDoneStr = "File systems created (";
static const char* const checkingAhciStr = "Checking AHCI controllers";
static const char* const checkingAhciDoneStr = "AHCI controllers checked\n";
static const char* const isoFoundStr = "ISO filesystem found at ";
static const char* const globalCtorStr = "Running global constructors";
static const char* const enabledInterruptsStr = "Enabled interrupts\n\n";

bool Kernel::debug = false;
InfoTable *Kernel::infoTable;

Async::Thenable<void> startPcieDrivers() {
	for (auto &function : PCIe::functions) {
		Async::Thenable<bool> (*initializer)(PCIe::Function &pcieFunction) = nullptr;
		if (
			function.configurationSpace->mainClass == PCIe::Class::Storage &&
			function.configurationSpace->subClass == PCIe::Subclass::Sata &&
			function.configurationSpace->progIf == PCIe::ProgramType::Ahci
		) {
			initializer = &AHCI::initialize;
		}
		if (initializer && !(co_await (*initializer)(function))) {
			Kernel::panic();
		}
	}
	co_return;
}

Async::Thenable<void> createFileSystems() {
	terminalPrintString(creatingFsStr, strlen(creatingFsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Create filesystems from AHCI devices
	terminalPrintSpaces4();
	terminalPrintString(checkingAhciStr, strlen(checkingAhciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	for (size_t controllerCount = 0; auto &controller : AHCI::controllers) {
		for (const auto &device : controller.getDevices()) {
			// Try with ISO9660 for SATAPI devices first because that is the most likely FS
			if (AHCI::Device::Type::Satapi == device->getType()) {
				auto primaryDescriptor = co_await FS::ISO9660::isIso9660(device);
				if (primaryDescriptor) {
					FS::filesystems.push_back(std::make_shared<FS::ISO9660>(device, primaryDescriptor));
					terminalPrintSpaces4();
					terminalPrintSpaces4();
					terminalPrintString(isoFoundStr, strlen(isoFoundStr));
					terminalPrintDecimal(controllerCount);
					terminalPrintChar(':');
					terminalPrintDecimal(device->getPortNumber());
					terminalPrintChar('\n');
				}
			}
		}
		++controllerCount;
	}
	terminalPrintSpaces4();
	terminalPrintString(checkingAhciDoneStr, strlen(checkingAhciDoneStr));

	// Create filesystems from other device types here when appropriate drivers are added

	terminalPrintString(creatingFsDoneStr, strlen(creatingFsDoneStr));
	terminalPrintDecimal(FS::filesystems.size());
	terminalPrintChar(')');
	terminalPrintChar('\n');
	co_return;
}

extern "C" [[noreturn]] void kernelMain(
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

	if (!enableSse4()) {
		Kernel::panic();
	}
	terminalPrintChar('\n');

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

	if (!ACPI::parse()) {
		Kernel::panic();
	}
	// Disable PIC and setup APIC
	if (!APIC::initialize()) {
		Kernel::panic();
	}

	initializePs2Keyboard();

	if (!Kernel::Scheduler::initialize()) {
		Kernel::panic();
	}

	enableInterrupts();
	terminalPrintString(enabledInterruptsStr, strlen(enabledInterruptsStr));

	// Enumerate PCIe devices
	if (!PCIe::enumerate()) {
		Kernel::panic();
	}

	// Start drivers for PCIe devices
	auto initResults = startPcieDrivers().then(&createFileSystems);

	// Wait perpetually and let the scheduler and interrupts do their thing
	Kernel::Scheduler::start();
}

void Kernel::panic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicStr, strlen(kernelPanicStr));
	Kernel::hangSystem();
}
