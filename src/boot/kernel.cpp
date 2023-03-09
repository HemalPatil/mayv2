#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/filesystems/jolietIso.h>
#include <drivers/ps2/keyboard.h>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
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
static const char* const sse4Str = "SSE4.2 enabled\n";
static const char* const checkRandStr = "Checking RDRAND presence";
static const char* const enableX2ApicStr = "Enabling x2APIC";
static const char* const rootFailStr = "Failed to find root filesystem\n";
static const char* const rootFoundStr = "Root filesystem found ";
static const char* const sipiSentStr = "SIPI sent\n";
static const char* const initApuStr = "Initializing";
static const char* const creatingTssStr = "Creating TSS";
static const char* const loadingIdtStr = "Loading IDT";

static Async::Thenable<void> startPcieDrivers();
static Async::Thenable<void> createFileSystems();
static Async::Thenable<void> findRootFs();
static Async::Thenable<void> bootApus();

bool Kernel::debug = false;
InfoTable Kernel::infoTable;

extern "C" [[noreturn]] void bpuMain(
	InfoTable *infoTableAddress,
	size_t kernelSize,
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
		kernelSize,
		phyMemBuddyPagesCount
	)) {
		Kernel::panic();
	}

	// Initialize virtual memory
	if (!Kernel::Memory::Virtual::initialize(
		(void*)(KERNEL_ORIGIN + kernelSize),
		phyMemBuddyPagesCount,
		globalCtors
	)) {
		Kernel::panic();
	}

	if (!ACPI::parse()) {
		Kernel::panic();
	}

	// Parse APIC table and create CPU entries
	if (!APIC::parse()) {
		Kernel::panic();
	}

	// Check the presence of x2APIC and enable it
	terminalPrintString(enableX2ApicStr, strlen(enableX2ApicStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!APIC::enableX2Apic()) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		Kernel::panic();
	}
	uint64_t spuriousInterruptVector = Kernel::readMsr(Kernel::MSR::x2ApicSpuriousInterrupt);
	spuriousInterruptVector &= 0xffffffffUL;
	spuriousInterruptVector |= 0x100;
	Kernel::writeMsr(Kernel::MSR::x2ApicSpuriousInterrupt, spuriousInterruptVector);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Get the boot CPU entry
	uint64_t bootApicId = Kernel::readMsr(Kernel::MSR::x2ApicId);
	for (auto &cpu : APIC::cpus) {
		if (cpu.apicId == bootApicId) {
			APIC::bootCpu = &cpu;
			APIC::bootCpu->apicPhyAddr = Kernel::readMsr(Kernel::MSR::x2ApicEnable) & 0xffffff000;
			break;
		}
	}
	if (!APIC::bootCpu) {
		Kernel::panic();
	}

	// Create TSS and install it
	terminalPrintString(creatingTssStr, strlen(creatingTssStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	const auto tss = Kernel::TSS::createAndInstall();
	uint16_t tssSelector = std::get<0>(tss);
	Kernel::TSS::Entry *tssEntry = std::get<1>(tss);
	if (tssSelector == 0xffff || !tssEntry) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		Kernel::panic();
	}
	APIC::bootCpu->tssSelector = tssSelector;
	APIC::bootCpu->intZone1 = (APIC::InterruptDataZone*)tssEntry->ist1Rsp;
	APIC::bootCpu->intZone1->apicId = bootApicId;
	APIC::bootCpu->intZone1->reserved0 = APIC::bootCpu->intZone1->reserved1 = 0;
	APIC::bootCpu->intZone2 = (APIC::InterruptDataZone*)tssEntry->ist2Rsp;
	APIC::bootCpu->intZone2->apicId = bootApicId;
	APIC::bootCpu->intZone2->reserved0 = APIC::bootCpu->intZone2->reserved1 = 0;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Load the common 64-bit IDT
	terminalPrintString(loadingIdtStr, strlen(loadingIdtStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!Kernel::IDT::setup()) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		Kernel::panic();
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	terminalPrintChar('\n');

	initializePs2Keyboard();

	if (!Kernel::Scheduler::start()) {
		Kernel::panic();
	}

	Kernel::IDT::enableInterrupts();
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
	Kernel::perpetualWait();
}

bool Kernel::IDT::setup() {
	bool installedExceptionHandlers =
		installEntry(0, divisionByZeroHandler, 1) &&
		installEntry(1, debugHandler, 1) &&
		installEntry(2, nmiHandler, 1) &&
		installEntry(3, breakpointHandler, 1) &&
		installEntry(4, overflowHandler, 1) &&
		installEntry(5, boundRangeHandler, 1) &&
		installEntry(6, invalidOpcodeHandler, 1) &&
		installEntry(7, noSseHandler, 1) &&
		installEntry(8, doubleFaultHandler, 1) &&
		installEntry(13, gpFaultHandler, 1) &&
		installEntry(14, pageFaultHandler, 1) &&
		installEntry(19, noSseHandler, 1);
	if (!installedExceptionHandlers) {
		return false;
	}
	loadIdt();
	return true;
}

bool Kernel::IDT::installEntry(uint8_t interruptNumber, void (*handler)(), uint8_t ist) {
	if (ist > 7) {
		return false;
	}
	Entry *entry = &idt64Base[interruptNumber];
	uint64_t handlerAddr = (uint64_t)handler;
	entry->offsetLow = handlerAddr & 0xffff;
	entry->segmentSelector = 0x8;
	entry->ist = ist;
	entry->reserved0 = 0;
	entry->type = 0xe;
	entry->reserved1 = 0;
	entry->desiredPrivilegeLevel = 0;
	entry->present = 1;
	entry->offsetMid = (handlerAddr >> 16) & 0xffff;
	entry->offsetHigh = handlerAddr >> 32;
	entry->reserved2 = 0;
	return true;
}

std::tuple<uint16_t, Kernel::TSS::Entry*> Kernel::TSS::createAndInstall() {
	using namespace Kernel::Memory;

	Entry *tss = new Entry();
	auto requestResult = Virtual::requestPages(
		2,
		(
			RequestType::AllocatePhysical |
			RequestType::CacheDisable |
			RequestType::Kernel |
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous |
			RequestType::Writable
		)
	);
	if (requestResult.allocatedCount != 2 || requestResult.address == INVALID_ADDRESS) {
		return { 0xffff, nullptr };
	}

	uint64_t istBase = (uint64_t)requestResult.address;
	tss->ist1Rsp = (void*)(istBase + pageSize - sizeof(APIC::InterruptDataZone));
	tss->ist2Rsp = (void*)(istBase + 2 * pageSize - sizeof(APIC::InterruptDataZone));

	const auto tssSelector = GDT::getAvailableSelector();
	const auto i = tssSelector / 8;
	const auto tssBase = (uint64_t)tss;
	GDT::gdt64Base[i].limitLow = 104;
	GDT::gdt64Base[i].baseLow = tssBase & 0xffff;
	GDT::gdt64Base[i].baseMid = (tssBase & 0xff0000) >> 16;
	GDT::gdt64Base[i].type = 9;
	GDT::gdt64Base[i].present = 1;
	GDT::gdt64Base[i].baseHigh = (tssBase & 0xff000000) >> 24;
	GDT::gdt64Base[i + 1].limitLow = (tssBase & 0xffff00000000) >> 32;
	GDT::gdt64Base[i + 1].baseLow = tssBase >> 48;
	loadTss(tssSelector);
	return { tssSelector, tss };
}

uint16_t Kernel::GDT::getAvailableSelector() {
	for (size_t i = 1; i < 512; ++i) {
		if (gdt64Base[i].present) {
			if (gdt64Base[i].type == 9) {
				// Skip next entry because this is a 16-byte wide 64-bit TSS descriptor
				++i;
			}
		} else {
			return i * 8;
		}
	}
	return 0;
}

extern "C" [[noreturn]] void apuMain() {
	// Wait perpetually and let the scheduler and interrupts do their thing
	Kernel::perpetualWait();
}

static Async::Thenable<void> bootApus() {
	if (1 == APIC::cpus.size()) {
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

	for (const auto &cpu : APIC::cpus) {
		if (cpu.apicId != APIC::bootCpu->apicId) {
			terminalPrintSpaces4();
			terminalPrintString(cpuStr, strlen(cpuStr));
			terminalPrintDecimal(cpu.apicId);
			terminalPrintChar(':');
			terminalPrintChar('\n');
			Kernel::writeMsr(Kernel::MSR::x2ApicErrorStatus, 0);
			Kernel::writeMsr(Kernel::MSR::x2ApicInterruptCommand, ((uint64_t)cpu.apicId << 32) | 0x4600 | (APU_BOOTLOADER_ORIGIN >> 12));
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
