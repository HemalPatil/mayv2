#include <commonstrings.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const char* const configuringStr = "Configuring port ";
static const char* const tfeStr = "AHCI task file error caused by commands ";
static const char* const tfeUnsolicitedStr = "AHCI unsolicited task file error ";
static const char* const d2hUnsolicitedStr = "AHCI unsolicited register D2H FIS ";
static const char* const atDeviceStr = "at device";

void AHCI::Device::msiHandler() {
	Device *thisDevice = this;
	uint32_t interruptStatus = this->port->interruptStatus;
	this->port->interruptStatus = interruptStatus;
	uint32_t completedCommands = ~this->port->commandIssue & this->runningCommands;
	terminalPrintString(" pis", 4);
	terminalPrintHex(&interruptStatus, sizeof(interruptStatus));
	terminalPrintHex((void*)&this->port->interruptStatus, sizeof(this->port->interruptStatus));

	if (interruptStatus & AHCI_TASK_FILE_ERROR) {
		// Find which command caused the error
		if (completedCommands) {
			terminalPrintString(tfeStr, strlen(tfeStr));
			terminalPrintHex(&completedCommands, sizeof(completedCommands));
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
			// terminalPrintString("cc", 2);
			for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
				// TODO: maybe check for received FIS status
				uint32_t commandBit = (uint32_t)1 << i;
				if (completedCommands & commandBit) {
					this->runningCommands &= ~commandBit;
					// TODO: should schedule the callback on some thread instead of sync execution
					if (
						this->commandCallbacks[i] != NULL &&
						this->commandCallbacks[i] != INVALID_ADDRESS
					) {
						(*this->commandCallbacks[i])();
					}
					this->commandCallbacks[i] = NULL;
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
	terminalPrintChar('\n');
}

bool AHCI::Device::identify() {
	terminalPrintString("iden1", 5);
	if (this->type == AHCI_PORT_TYPE_NONE) {
		return false;
	}
	terminalPrintString("iden2", 5);

	this->info = new IdentifyDeviceData();
	PML4CrawlResult crawl(this->info);
	if (crawl.physicalTables[0] == INVALID_ADDRESS || crawl.indexes[0] % 2 != 0) {
		return false;
	}
	uint64_t bufferPhyAddr = (uint64_t)crawl.physicalTables[0] + crawl.indexes[0];

	size_t freeSlot = this->findFreeCommandSlot();
	if (freeSlot == SIZE_MAX) {
		return false;
	}
	this->commandHeaders[freeSlot].prdtLength = 1;
	this->commandHeaders[freeSlot].commandFisLength = sizeof(FIS::RegisterH2D) / sizeof(uint32_t);
	this->commandHeaders[freeSlot].atapi = (this->type == AHCI_PORT_TYPE_SATAPI) ? 1 : 0;
	this->commandHeaders[freeSlot].write = 0;
	terminalPrintString("iden", 4);

	memset(this->commandTables[freeSlot], 0, sizeof(CommandTable) + (this->commandHeaders[freeSlot].prdtLength - 1) * sizeof(PRDTEntry));
	this->commandTables[freeSlot]->prdtEntries[0].dataBase = (uint32_t)bufferPhyAddr;
	if (this->controller->hba->hostCapabilities.bit64Addressing) {
		this->commandTables[freeSlot]->prdtEntries[0].dataBaseUpper = (uint32_t)(bufferPhyAddr >> 32);
	}
	this->commandTables[freeSlot]->prdtEntries[0].byteCount = 511;
	this->commandTables[freeSlot]->prdtEntries[0].interruptOnCompletion = 1;

	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
	memset(commandFis, 0, sizeof(FIS::RegisterH2D));
	commandFis->fisType = AHCI_FIS_TYPE_REG_H2D;
	commandFis->commandControl = 1;
	commandFis->command = (this->type == AHCI_PORT_TYPE_SATAPI) ? AHCI_IDENTIFY_PACKET_DEVICE : AHCI_IDENTIFY_DEVICE;

	this->runningCommands |= 1 << freeSlot;
	this->port->commandIssue = 1 << freeSlot;

	return true;
}

bool AHCI::Device::initialize() {
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(configuringStr, strlen(configuringStr));
	terminalPrintDecimal(this->portNumber);
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
	PageRequestResult requestResult = requestVirtualPages(
		1,
		(
			MEMORY_REQUEST_KERNEL_PAGE |
			MEMORY_REQUEST_CONTIGUOUS |
			MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE |
			MEMORY_REQUEST_CACHE_DISABLE
		)
	);
	if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
		return false;
	}
	PML4CrawlResult crawlResult(requestResult.address);
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
	// Each command tables's size is CommandFIS(64 bytes) + ACMD(16 bytes) + reserved(48 bytes) + prdtLength * PRDT entry size(16 bytes)
	// To make all the 32 command tables fit nicely in 2 pages, solve the equation for prdtLength
	// 2 * 4096 = 32 * (64 + 16 + 48 + prdtLength * 16)
	// Hence, prdtLength works out to 8
	// FIXME: requestVirtualPages does not guarantee the physical pages alloted will be contiguous
	requestResult = requestVirtualPages(
		2,
		MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS | MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE | MEMORY_REQUEST_CACHE_DISABLE
	);
	if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 2) {
		return false;
	}
	crawlResult = PML4CrawlResult(requestResult.address);
	phyAddr = (uint64_t)crawlResult.physicalTables[0];
	virAddr = (uint64_t)requestResult.address;
	memset(requestResult.address, 0, 2 * pageSize);
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

size_t AHCI::Device::findFreeCommandSlot() {
	terminalPrintString("\nfind", 5);
	uint32_t slots = this->port->commandIssue | this->port->sataActive | this->runningCommands;
	terminalPrintHex(&slots, sizeof(slots));
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
		if ((slots & 1) == 0) {
			terminalPrintString("findd\n", 6);
			return i;
		}
		slots >>= 1;
	}
	terminalPrintString("findd\n", 6);
	return SIZE_MAX;
}

AHCI::Device::Device(Controller *controller) {
	this->type = AHCI_PORT_TYPE_NONE;
	this->portNumber = UINT8_MAX;
	this->port = NULL;
	this->commandHeaders = NULL;
	this->fisBase = NULL;
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader); ++i) {
		this->commandTables[i] = NULL;
		this->commandCallbacks[i] = NULL;
	}
	this->runningCommands = 0;
	this->info = NULL;
	this->controller = controller;
}
