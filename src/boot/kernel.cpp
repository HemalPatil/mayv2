#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/filesystems/jolietIso.h>
#include <drivers/ps2/keyboard.h>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <idt64.h>
#include <kernel.h>
#include <pcie.h>
#include <random.h>
#include <sse4.h>
#include <terminal.h>
#include <tss64.h>
// #include <vbe.h>

static const char* const kernelLoadedStr = "Kernel loaded\nRunning in 64-bit long mode\n\n";
static const char* const kernelPanicStr = "\n!!! Kernel panic !!!\n!!! Halting the system !!!\n";
static const char* const creatingFsStr = "Creating file systems";
static const char* const creatingFsDoneStr = "File systems created [";
static const char* const atAhciStr = "on AHCI device";
static const char* const isoFoundStr = "JolietISO";
static const char* const globalCtorStr = "Running global constructors";
static const char* const enabledInterruptsStr = "Enabled interrupts\n\n";
static const char* const apuBootStr = "Loading auxiliary CPU bootstrap binary";
static const char* const initApusStr = "Initializing CPUs";
static const char* const cpuStr = "CPU ";
static const char* const noFsStr = "Expected to find at least 1 filesystem\n";
static const char* const sse4Str = "SSE4.2 enabled\n\n";
static const char* const checkRandStr = "Checking RDRAND presence";
static const char* const rootFailStr = "Failed to find root filesystem\n";
static const char* const rootFoundStr = "Root filesystem found ";
static const char* const sipiSentStr = "SIPI sent\n";
static const char* const initApuStr = "Initializing";

static Async::Thenable<void> startPcieDrivers();
static Async::Thenable<void> createFileSystems();
static Async::Thenable<void> findRootFs();
static Async::Thenable<void> bootApus();

bool Kernel::debug = false;
InfoTable Kernel::infoTable;

extern "C" [[noreturn]] void kernelMain(
	InfoTable *infoTableAddress,
	size_t lowerHalfSize,
	size_t higherHalfSize,
	void* usablePhyMemStart
) {
	// Be careful of nullptr references until
	// the virtual memory manager is initialized
	// and it has unmapped the first page

	// Enable SSE4 first so the rest of the kernel optimizations
	// can make use of it, otherwise system crashes
	if (!enableSse4()) {
		Kernel::panic();
	}

	// Copy the infoTable to kernel address space so it can still be accessed
	// after the first page has been unmapped to detect nullptr accesses
	memcpy(&Kernel::infoTable, infoTableAddress, sizeof(InfoTable));
	Kernel::GlobalConstructor globalCtors[Kernel::infoTable.globalCtorsCount];
	memcpy(
		globalCtors,
		(void*)Kernel::infoTable.globalCtorsLocation,
		Kernel::infoTable.globalCtorsCount * sizeof(uint64_t)
	);

	terminalSetBgColour(TERMINAL_COLOUR_BLUE);
	terminalSetTextColour(TERMINAL_COLOUR_BWHITE);
	terminalClearScreen();
	terminalSetCursorPosition(0, 0);
	terminalPrintString(kernelLoadedStr, strlen(kernelLoadedStr));
	terminalPrintString(sse4Str, strlen(sse4Str));

	terminalPrintString(checkRandStr, strlen(checkRandStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!Random::isRandomAvailable()) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		Kernel::panic();
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');
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
			.then(findRootFs)
			.then(bootApus);
	#pragma GCC diagnostic pop

	// Wait perpetually and let the scheduler and interrupts do their thing
	while (true) {
		Kernel::haltSystem();
	}
}

static Async::Thenable<void> bootApus() {
	if (1 == APIC::cpuEntries.size()) {
		co_return;
	}

	terminalPrintString(initApusStr, strlen(initApusStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Get the APU stage1 boot binary from root FS
	terminalPrintSpaces4();
	terminalPrintString(apuBootStr, strlen(apuBootStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	const auto fileReadResult = std::move(co_await FS::root->readFile("/boot/stage1/apu.bin"));
	if (fileReadResult.status == FS::Status::Ok && fileReadResult.data) {
		memcpy((void*)APU_BOOTLOADER_ORIGIN, fileReadResult.data.getData(), fileReadResult.data.getSize());
		Kernel::prepareApuInfoTable((ApuInfoTable*)(APU_BOOTLOADER_ORIGIN + APU_BOOTLOADER_PADDING), Kernel::infoTable.pml4tPhysicalAddress);
	} else {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		Kernel::panic();
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	const auto localApic = APIC::getLocal();
	for (const auto &cpu : APIC::cpuEntries) {
		if (cpu.cpuId != APIC::bootCpuId) {
			terminalPrintSpaces4();
			terminalPrintString(cpuStr, strlen(cpuStr));
			terminalPrintDecimal(cpu.cpuId);
			terminalPrintChar(':');
			terminalPrintChar('\n');
			localApic->errorStatus = 0;
			localApic->interruptCommandHigh = (localApic->interruptCommandHigh & 0x00ffffff) | (((uint32_t)cpu.cpuId) << 24);
			localApic->interruptCommandLow = (localApic->interruptCommandLow & 0xfff32000) | (0x4600 | (APU_BOOTLOADER_ORIGIN >> 12));
			terminalPrintSpaces4();
			terminalPrintSpaces4();
			terminalPrintString(sipiSentStr, strlen(sipiSentStr));
			terminalPrintSpaces4();
			terminalPrintSpaces4();
			terminalPrintString(initApuStr, strlen(initApuStr));
			terminalPrintString(ellipsisStr, strlen(ellipsisStr));
			terminalPrintString(doneStr, strlen(doneStr));
			terminalPrintChar('\n');
		}
	}

	co_return;
}

static Async::Thenable<void> startPcieDrivers() {
	for (const auto &function : PCIe::functions) {
		Async::Thenable<bool> (*initializer)(const PCIe::Function &pcieFunction) = nullptr;
		if (
			PCIe::Class::Storage == function.configurationSpace->mainClass &&
			PCIe::Subclass::Sata == function.configurationSpace->subClass &&
			PCIe::ProgramType::Ahci == function.configurationSpace->progIf
		) {
			initializer = &AHCI::initialize;
		}
		if (initializer && !(co_await (*initializer)(function))) {
			Kernel::panic();
		}
	}
	co_return;
}

static Async::Thenable<void> findRootFs() {
	for (const auto &fs : FS::filesystems) {
		const auto bootDirResult = std::move(co_await fs->readDirectory("/boot/"));
		if (bootDirResult.status == FS::Status::Ok) {
			for (const auto &file : bootDirResult.entries) {
				if (
					file.isFile &&
					file.name == std::string(Kernel::infoTable.rootFsGuid) + ".root-fs"
				) {
					FS::root = fs;
					break;
				}
			}
		} else {
			terminalPrintString(rootFailStr, strlen(rootFailStr));
			Kernel::panic();
		}
	}
	if (FS::root) {
		terminalPrintString(rootFoundStr, strlen(rootFoundStr));
		FS::root->getGuid().print(true);
		terminalPrintChar('\n');
		terminalPrintChar('\n');
	} else {
		terminalPrintString(rootFailStr, strlen(rootFailStr));
		Kernel::panic();
	}
	co_return;
}

static Async::Thenable<void> createFileSystems() {
	terminalPrintString(creatingFsStr, strlen(creatingFsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Create filesystems from AHCI devices
	for (size_t controllerCount = 0; const auto &controller : AHCI::controllers) {
		for (const auto &device : controller.getDevices()) {
			// Try with JolietISO for SATAPI devices first because that is the most likely FS
			if (AHCI::Device::Type::Satapi == device->getType()) {
				auto svd = std::move(co_await FS::JolietISO::isJolietIso(device));
				if (svd) {
					std::shared_ptr<FS::JolietISO> iso = std::make_shared<FS::JolietISO>(device, svd);
					if (co_await iso->initialize()) {
						FS::filesystems.push_back(iso);
						terminalPrintSpaces4();
						iso->getGuid().print(true);
						terminalPrintChar(' ');
						terminalPrintString(isoFoundStr, strlen(isoFoundStr));
						terminalPrintChar(' ');
						terminalPrintString(atAhciStr, strlen(atAhciStr));
						terminalPrintChar(' ');
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
