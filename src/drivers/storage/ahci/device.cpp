#include <commonstrings.h>
#include <cstring>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <kernel.h>
#include <terminal.h>

static const char* const commandNamespaceStr = "AHCI::Device::Command";
static const char* const multiSetStr = " tried setting result multiple times\n";
static const char* const noResultStr = " result not available\n";
static const char* const initStr = "Initializing";
static const char* const identStr = "Identifying";
static const char* const tfeStr = "AHCI task file error caused by commands ";
static const char* const tfeUnsolicitedStr = "AHCI unsolicited task file error ";
static const char* const d2hUnsolicitedStr = "AHCI unsolicited register D2H FIS ";
static const char* const atDeviceStr = "at device";

Drivers::Storage::Buffer Drivers::Storage::AHCI::Device::setupRead(size_t blockCount, size_t &freeSlot) {
	if (blockCount == 0) {
		return nullptr;
	}

	// Each command has 8 PRDTs (read AHCI::Device::initialize comments to know why 8 PRDTs)
	// and each PRDT can handle 4MiB i.e. maximum of 32MiB
	const size_t mib4 = 2UL * MIB_2;
	const size_t totalBytes = blockCount * this->blockSize;
	if (totalBytes > 8 * mib4) {
		return nullptr;
	}

	freeSlot = this->findFreeCommandSlot();
	if (freeSlot == SIZE_MAX) {
		return nullptr;
	}

	size_t prdsRequired = totalBytes / mib4;
	if (totalBytes > prdsRequired * mib4) {
		++prdsRequired;
	}

	// Refer section 5.5.1 and 5.6.2.4 of AHCI specification https://www.intel.com.au/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
	this->commandHeaders[freeSlot].prdtLength = prdsRequired;
	this->commandHeaders[freeSlot].commandFisLength = sizeof(FIS::RegisterH2D) / sizeof(uint32_t);
	this->commandHeaders[freeSlot].write = 0;

	// Clear out the command table
	memset(this->commandTables[freeSlot], 0, sizeof(CommandTable) + prdsRequired * sizeof(PRDTEntry));

	// Setup PRDT entries
	// First (prdsRequired - 1) will all have byteCount set to (mib4 - 1) and will not interrupt.
	// The last PRDT entry will have remaining byteCount and must interrupt to signal command completion.
	Storage::Buffer buffer = Storage::Buffer(totalBytes, AHCI_BUFFER_ALIGN_AT);
	uint64_t bufferSegmentPhyAddr;
	for (size_t i = 0; i < prdsRequired - 1; ++i) {
		bufferSegmentPhyAddr = buffer.getPhysicalAddress() + i * mib4;
		this->commandTables[freeSlot]->prdtEntries[i].dataBase = (uint32_t)(bufferSegmentPhyAddr);
		if (controller->hba->hostCapabilities.bit64Addressing) {
			this->commandTables[freeSlot]->prdtEntries[i].dataBaseUpper = (uint32_t)(bufferSegmentPhyAddr >> 32);
		}
		this->commandTables[freeSlot]->prdtEntries[i].byteCount = mib4 - 1;
		this->commandTables[freeSlot]->prdtEntries[i].interruptOnCompletion = 0;
	}
	bufferSegmentPhyAddr = buffer.getPhysicalAddress() + (prdsRequired - 1) * mib4;
	this->commandTables[freeSlot]->prdtEntries[prdsRequired - 1].dataBase = (uint32_t)bufferSegmentPhyAddr;
	if (controller->hba->hostCapabilities.bit64Addressing) {
		this->commandTables[freeSlot]->prdtEntries[prdsRequired - 1].dataBaseUpper = (uint32_t)(bufferSegmentPhyAddr >> 32);
	}
	this->commandTables[freeSlot]->prdtEntries[prdsRequired - 1].byteCount = totalBytes - (prdsRequired - 1) * mib4;
	this->commandTables[freeSlot]->prdtEntries[prdsRequired - 1].interruptOnCompletion = 1;

	// Setup the command FIS
	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
	memset(commandFis, 0, sizeof(FIS::RegisterH2D));
	commandFis->fisType = AHCI_FIS_TYPE_REG_H2D;
	commandFis->commandControl = 1;
	return buffer;
}

void Drivers::Storage::AHCI::Device::msiHandler() {
	// FIXME: should lock the block device
	auto *thisDevice = this;
	uint32_t interruptStatus = this->port->interruptStatus;
	this->port->interruptStatus = interruptStatus;
	uint32_t completedCommands = ~this->port->commandIssue & this->runningCommandsBitmap;

	if (interruptStatus & AHCI_TASK_FILE_ERROR) {
		// Find which command caused the error
		if (completedCommands) {
			terminalPrintString(tfeStr, strlen(tfeStr));
			terminalPrintHex(&completedCommands, sizeof(completedCommands));
			for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
				uint32_t commandBit = (uint32_t)1 << i;
				if (completedCommands & commandBit) {
					this->runningCommandsBitmap &= ~commandBit;
					if (this->commands[i]) {
						this->commands[i]->setResult(false);
						this->commands[i] = nullptr;
					}
				}
			}
		} else {
			terminalPrintString(tfeUnsolicitedStr, strlen(tfeUnsolicitedStr));
		}
		terminalPrintString(atDeviceStr, strlen(atDeviceStr));
		terminalPrintChar(' ');
		terminalPrintHex(&thisDevice, sizeof(thisDevice));
		terminalPrintChar('\n');
	}

	if (interruptStatus & AHCI_REGISTER_D2H) {
		if (completedCommands) {
			for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
				// TODO: maybe check for received FIS status
				uint32_t commandBit = (uint32_t)1 << i;
				if (completedCommands & commandBit) {
					this->runningCommandsBitmap &= ~commandBit;
					if (this->commands[i]) {
						this->commands[i]->setResult(true);
						this->commands[i] = nullptr;
					}
				}
			}
		} else {
			terminalPrintString(d2hUnsolicitedStr, strlen(d2hUnsolicitedStr));
			terminalPrintString(atDeviceStr, strlen(atDeviceStr));
			terminalPrintChar(' ');
			terminalPrintHex(&thisDevice, sizeof(thisDevice));
			terminalPrintChar('\n');
		}
	}
}

Async::Thenable<bool> Drivers::Storage::AHCI::Device::identify() {
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(identStr, strlen(identStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	this->info = new IdentifyDeviceData();
	Kernel::Memory::Virtual::CrawlResult crawl(this->info);
	if (crawl.physicalTables[0] == INVALID_ADDRESS || crawl.indexes[0] % 2 != 0) {
		co_return false;
	}
	uint64_t bufferPhyAddr = (uint64_t)crawl.physicalTables[0] + crawl.indexes[0];

	size_t freeSlot = this->findFreeCommandSlot();
	if (freeSlot == SIZE_MAX) {
		co_return false;
	}
	this->commandHeaders[freeSlot].prdtLength = 1;
	this->commandHeaders[freeSlot].commandFisLength = sizeof(FIS::RegisterH2D) / sizeof(uint32_t);
	this->commandHeaders[freeSlot].atapi = (this->type == Type::Satapi) ? 1 : 0;
	this->commandHeaders[freeSlot].write = 0;

	memset(this->commandTables[freeSlot], 0, sizeof(CommandTable) + (this->commandHeaders[freeSlot].prdtLength - 1) * sizeof(PRDTEntry));
	this->commandTables[freeSlot]->prdtEntries[0].dataBase = (uint32_t)bufferPhyAddr;
	if (this->controller->hba->hostCapabilities.bit64Addressing) {
		this->commandTables[freeSlot]->prdtEntries[0].dataBaseUpper = (uint32_t)(bufferPhyAddr >> 32);
	}
	this->commandTables[freeSlot]->prdtEntries[0].byteCount = sizeof(IdentifyDeviceData) - 1;
	this->commandTables[freeSlot]->prdtEntries[0].interruptOnCompletion = 1;

	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
	memset(commandFis, 0, sizeof(FIS::RegisterH2D));
	commandFis->fisType = AHCI_FIS_TYPE_REG_H2D;
	commandFis->commandControl = 1;
	commandFis->command = (this->type == Type::Satapi) ? AHCI_IDENTIFY_PACKET_DEVICE : AHCI_IDENTIFY_DEVICE;

	if (co_await Command(this, freeSlot)) {
		// Read section 7.16.7, 7.16.7.54, 7.16.7.59 of ATA8-ACS spec (https://people.freebsd.org/~imp/asiabsdcon2015/works/d2161r5-ATAATAPI_Command_Set_-_3.pdf)
		// to understand how physical and logical sector size can be determined
		if (this->info->physicalLogicalSectorSize.valid) {
			// FIXME: complete sector size determination
		} else {
			// Assume sector size of 512 bytes for SATA and 2048 bytes for SATAPI
			this->blockSize = this->type == Type::Sata ? 512 : 2048;
		}
		terminalPrintString(doneStr, strlen(doneStr));
		terminalPrintChar('\n');
		co_return true;
	}

	co_return false;
}

bool Drivers::Storage::AHCI::Device::initialize() {
	using namespace Kernel::Memory;

	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(initStr, strlen(initStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	// Busy wait till the HBA is receiving FIS or running some command
	this->port->commandStatus.start = 0;
	this->port->commandStatus.fisReceiveEnable = 0;
	while (true) {
		if (
			!this->port->commandStatus.fisReceiveRunning &&
			!this->port->commandStatus.commandListRunning
		) {
			break;
		}
	}

	// Request a page where the command list (1024 bytes) and received FISes (256 bytes) can be placed
	// Get the physical address of this page and put it in the commandListBase and fisBase
	PageRequestResult requestResult = Virtual::requestPages(
		1,
		(
			RequestType::Kernel |
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous |
			RequestType::AllocatePhysical |
			RequestType::CacheDisable |
			RequestType::Writable
		)
	);
	if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
		return false;
	}
	Virtual::CrawlResult crawlResult(requestResult.address);
	uint64_t phyAddr = (uint64_t)crawlResult.physicalTables[0];
	uint64_t virAddr = (uint64_t)requestResult.address;
	memset(requestResult.address, 0, pageSize);
	this->commandHeaders = (CommandHeader*)virAddr;
	this->port->commandListBase = (uint32_t)phyAddr;
	if (this->controller->hba->hostCapabilities.bit64Addressing) {
		this->port->commandListBaseUpper = (uint32_t)(phyAddr >> 32);
	}
	phyAddr += AHCI_COMMAND_LIST_SIZE;
	virAddr += AHCI_COMMAND_LIST_SIZE;
	this->fisBase = (void*)virAddr;
	this->port->fisBase = (uint32_t)phyAddr;
	if (this->controller->hba->hostCapabilities.bit64Addressing) {
		this->port->fisBaseUpper = (uint32_t)(phyAddr >> 32);
	}

	// There are AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader) i.e. 32 command tables
	// PRDT entry size is 16 bytes
	// Each command table's size is CommandFIS(64 bytes) + ACMD(16 bytes) + reserved(48 bytes) + prdtLength * PRDT entry size(16 bytes)
	// To make all the 32 command tables fit nicely in 2 pages, solve the equation for prdtLength
	// 2 * 4096 = 32 * (64 + 16 + 48 + prdtLength * 16)
	// Hence, prdtLength works out to 8
	requestResult = Virtual::requestPages(
		2,
		(
			RequestType::Kernel |
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous |
			RequestType::AllocatePhysical |
			RequestType::CacheDisable |
			RequestType::Writable
		)
	);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 2
	) {
		return false;
	}
	crawlResult = Kernel::Memory::Virtual::CrawlResult(requestResult.address);
	phyAddr = (uint64_t)crawlResult.physicalTables[0];
	virAddr = (uint64_t)requestResult.address;
	memset(requestResult.address, 0, 2 * Kernel::Memory::pageSize);
	const size_t commandTableSize = 64 + 16 + 48 + 16 * 8;
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
		this->commandHeaders[i].prdtLength = 8;
		this->commandTables[i] = (CommandTable*)virAddr;
		this->commandHeaders[i].commandTableBase = (uint32_t)phyAddr;
		if (this->controller->hba->hostCapabilities.bit64Addressing) {
			this->commandHeaders[i].commandTableBaseUpper = (uint32_t)(phyAddr >> 32);
		}
		phyAddr += commandTableSize;
		virAddr += commandTableSize;
	}
	this->port->interruptEnable = (AHCI_TASK_FILE_ERROR | AHCI_REGISTER_D2H);

	// Busy wait till the HBA is running some command
	while (true) {
		if (!this->port->commandStatus.commandListRunning) {
			break;
		}
	}
	this->port->commandStatus.fisReceiveEnable = 1;
	this->port->commandStatus.start = 1;

	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	return true;
}

size_t Drivers::Storage::AHCI::Device::findFreeCommandSlot() const {
	uint32_t slots = this->port->commandIssue | this->port->sataActive | this->runningCommandsBitmap;
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
		if ((slots & 1) == 0) {
			return i;
		}
		slots >>= 1;
	}
	return SIZE_MAX;
}

Drivers::Storage::AHCI::Device::Type Drivers::Storage::AHCI::Device::getType() const {
	return this->type;
}

size_t Drivers::Storage::AHCI::Device::getPortNumber() const {
	return this->portNumber;
}

Drivers::Storage::AHCI::Device::Device(Controller *controller, size_t portNumber) {
	this->type = Type::None;
	this->portNumber = portNumber;
	this->port = &controller->hba->ports[portNumber];
	this->commandHeaders = nullptr;
	this->fisBase = nullptr;
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
		this->commandTables[i] = nullptr;
	}
	this->runningCommandsBitmap = 0;
	this->info = nullptr;
	this->controller = controller;
}

Drivers::Storage::AHCI::Device::Command::Command(Device *device, size_t freeSlot)
	:	device(device),
		freeSlot(freeSlot),
		awaitingCoroutine(nullptr),
		result(nullptr) {}

Drivers::Storage::AHCI::Device::Command::~Command() {
	if (this->awaitingCoroutine) {
		delete this->awaitingCoroutine;
		this->awaitingCoroutine = nullptr;
	}
	if (this->result) {
		delete result;
		this->result = nullptr;
	}
}

void Drivers::Storage::AHCI::Device::Command::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
	// FIXME: should lock the block device
	this->awaitingCoroutine = new std::coroutine_handle<>(awaitingCoroutine);
	this->device->commands[freeSlot] = this;
	this->device->runningCommandsBitmap |= 1 << freeSlot;
	this->device->port->commandIssue = 1 << this->freeSlot;
}

bool Drivers::Storage::AHCI::Device::Command::await_resume() const noexcept {
	if (!this->result) {
		terminalPrintString(commandNamespaceStr, strlen(commandNamespaceStr));
		terminalPrintString(noResultStr, strlen(noResultStr));
		Kernel::panic();
	}
	return *this->result;
}

void Drivers::Storage::AHCI::Device::Command::setResult(bool result) noexcept {
	if (this->result) {
		terminalPrintString(commandNamespaceStr, strlen(commandNamespaceStr));
		terminalPrintString(multiSetStr, strlen(multiSetStr));
		Kernel::panic();
	}
	this->result = new bool(result);
	if (this->awaitingCoroutine) {
		Kernel::Scheduler::queueEvent(*this->awaitingCoroutine);
	}
}
