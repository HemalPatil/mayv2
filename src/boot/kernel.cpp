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
static const char* const creatingFsDoneStr = "File systems created [";
static const char* const checkingAhciStr = "Checking AHCI controllers";
static const char* const checkingAhciDoneStr = "AHCI controllers checked\n";
static const char* const isoFoundStr = "ISO filesystem found at ";
static const char* const globalCtorStr = "Running global constructors";
static const char* const enabledInterruptsStr = "Enabled interrupts\n\n";
static const char* const apuBootStr = "Loading auxiliary CPU bootstrap binary";
static const char* const initApusStr = "Initializing CPUs";
static const char* const cpuStr = "CPU ";
static const char* const noFsStr = "Expected to find at least 1 filesystem\n";
static const char* const sse4Str = "SSE4 enabled\n\n";

static Async::Thenable<void> startPcieDrivers();
static Async::Thenable<void> createFileSystems();
static Async::Thenable<void> bootApus();

bool Kernel::debug = false;
InfoTable *Kernel::infoTable = nullptr;

extern "C" [[noreturn]] void kernelMain(
	InfoTable *infoTableAddress,
	size_t lowerHalfSize,
	size_t higherHalfSize,
	void* usablePhyMemStart
) {
	// Be careful of nullptr references until the IVTs in
	// identity mapped page 0 of virtual address space
	// are moved somewhere else during interrupt initialization

	// Enable SSE4 first so the rest of the kernel optimizations
	// can make use of it, otherwise system crashes
	if (!enableSse4()) {
		Kernel::panic();
	}
	terminalPrintString(sse4Str, strlen(sse4Str));

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

	if (!ACPI::parse()) {
		Kernel::panic();
	}

	// Disable PIC and setup APIC
	if (!APIC::initialize()) {
		Kernel::panic();
	}

	initializePs2Keyboard();

	if (!Kernel::Scheduler::start()) {
		Kernel::panic();
	}

	enableInterrupts();
	terminalPrintString(enabledInterruptsStr, strlen(enabledInterruptsStr));

	// Enumerate PCIe devices
	if (!PCIe::enumerate()) {
		Kernel::panic();
	}

	// Start drivers for PCIe devices
	// Ignore the compiler warning since it is required to keep the variable in scope
	// and let all the async tasks finish
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	auto initResults =
		startPcieDrivers()
			.then(createFileSystems)
			.then(bootApus);
	#pragma GCC diagnostic pop

	// Wait perpetually and let the scheduler and interrupts do their thing
	while (true) {
		Kernel::haltSystem();
	}
}

static Async::Thenable<void> bootApus() {
	if (APIC::cpuEntries.size() == 1) {
		co_return;
	}

	// Search for APU stage1 boot binary in all the created filesystems
	terminalPrintString(initApusStr, strlen(initApusStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	terminalPrintSpaces4();
	terminalPrintString(apuBootStr, strlen(apuBootStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	for (const auto &fs : FS::filesystems) {
		terminalPrintChar('\n');
		for (const auto &dir : co_await fs->readDirectory("/BOOT.CAT/")) {
			terminalPrintString(dir.name.c_str(), dir.name.length());
			terminalPrintChar(' ');
			terminalPrintDecimal(dir.isFile);
			terminalPrintChar(' ');
			terminalPrintDecimal(dir.isDir);
			terminalPrintChar(' ');
			terminalPrintDecimal(dir.isSymLink);
			terminalPrintChar(' ');
			terminalPrintDecimal(dir.lba);
			terminalPrintChar(' ');
			terminalPrintDecimal(dir.size);
			terminalPrintChar('\n');
		}
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	for (const auto &cpu : APIC::cpuEntries) {
		if (cpu.cpuId != APIC::bootCpuId) {
			terminalPrintSpaces4();
			terminalPrintString(cpuStr, strlen(cpuStr));
			terminalPrintDecimal(cpu.cpuId);
			terminalPrintChar(':');
			terminalPrintChar('\n');
		}
	}

	co_return;
}

static Async::Thenable<void> startPcieDrivers() {
	for (const auto &function : PCIe::functions) {
		Async::Thenable<bool> (*initializer)(const PCIe::Function &pcieFunction) = nullptr;
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

static Async::Thenable<void> createFileSystems() {
	terminalPrintString(creatingFsStr, strlen(creatingFsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Create filesystems from AHCI devices
	terminalPrintSpaces4();
	terminalPrintString(checkingAhciStr, strlen(checkingAhciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	for (size_t controllerCount = 0; const auto &controller : AHCI::controllers) {
		for (const auto &device : controller.getDevices()) {
			// Try with ISO9660 for SATAPI devices first because that is the most likely FS
			if (AHCI::Device::Type::Satapi == device->getType()) {
				auto primaryDescriptor = std::move(co_await FS::ISO9660::isIso9660(device));
				if (primaryDescriptor) {
					std::shared_ptr<FS::ISO9660> iso = std::make_shared<FS::ISO9660>(device, primaryDescriptor);
					if (co_await iso->initialize()) {
						FS::filesystems.push_back(iso);
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
		}
		++controllerCount;
	}
	terminalPrintSpaces4();
	terminalPrintString(checkingAhciDoneStr, strlen(checkingAhciDoneStr));

	// Create filesystems from other device types here when appropriate drivers are added

	if (0 == FS::filesystems.size()) {
		terminalPrintString(noFsStr, strlen(noFsStr));
		Kernel::panic();
	}

	terminalPrintString(creatingFsDoneStr, strlen(creatingFsDoneStr));
	terminalPrintDecimal(FS::filesystems.size());
	terminalPrintChar(']');
	terminalPrintChar('\n');
	terminalPrintChar('\n');
	co_return;
}

void Kernel::panic() {
	// TODO : improve kernel panic implementation
	terminalPrintString(kernelPanicStr, strlen(kernelPanicStr));
	Kernel::hangSystem();
}
