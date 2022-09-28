#include <apic.h>
#include <commonstrings.h>
#include <drivers/storage/ahci.h>
#include <heapmemmgmt.h>
#include <idt64.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const char* const initAhciStr = "Initializing AHCI";
static const char* const initAhciCompleteStr = "AHCI initialized\n\n";
static const char* const mappingHbaStr = "Mapping HBA control registers to kernel address space";
static const char* const ahciSwitchStr = "Switching to AHCI mode";
static const char* const probingPortsStr = "Enumerating and configuring ports";
static const char* const configuringStr = "Configuring port ";
static const char* const configuredStr = "Ports configured";
static const char* const checkMsiStr = "Checking MSI capability";
static const char* const tfeStr = "AHCI task file error caused by commands ";
static const char* const tfeUnsolicitedStr = "AHCI unsolicited task file error ";
static const char* const d2hUnsolicitedStr = "AHCI unsolicited register D2H FIS ";
static const char* const atDeviceStr = "at device";

bool ahciAtapiRead(
	AHCIController *controller,
	AHCIDevice *device,
	size_t startSector,
	size_t sectorCount,
	void *buffer,
	void (*callback)()
);

AHCIController *ahciControllers = NULL;

void callback() {
	terminalPrintString("callback", 8);
}

void callback2() {
	terminalPrintString("callback2", 9);
}

void ahciDeviceMsiHandler(AHCIDevice *device) {
	uint32_t interruptStatus = device->port->interruptStatus;
	device->port->interruptStatus = interruptStatus;
	uint32_t completedCommands = ~device->port->commandIssue & device->runningCommands;
	terminalPrintHex(&interruptStatus, sizeof(interruptStatus));
	terminalPrintHex(&device->port->interruptStatus, sizeof(device->port->interruptStatus));

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
		terminalPrintHex(&device, sizeof(device));
		terminalPrintChar('\n');
	}

	if (interruptStatus & AHCI_REGISTER_D2H) {
		if (completedCommands) {
			// terminalPrintString("cc", 2);
			for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader); ++i) {
				// TODO: maybe check for received FIS status
				uint32_t commandBit = (uint32_t)1 << i;
				if (completedCommands & commandBit) {
					device->runningCommands &= ~commandBit;
					// TODO: should schedule the callback on some thread instead of sync execution
					(*device->commandCallbacks[i])();
					device->commandCallbacks[i] = NULL;
				}
			}
		} else {
			terminalPrintString(d2hUnsolicitedStr, strlen(d2hUnsolicitedStr));
			terminalPrintString(atDeviceStr, strlen(atDeviceStr));
			terminalPrintChar(' ');
			terminalPrintHex(&device, sizeof(device));
			terminalPrintChar('\n');
		}
	}
	terminalPrintChar('\n');
}

void ahciMsiHandler() {
	AHCIController *currentController = ahciControllers;
	terminalPrintString("msi", 3);
	while (currentController) {
		uint32_t interruptStatus = currentController->hba->interruptStatus;
		if (interruptStatus) {
			// If any port needs servicing write its value back to hba->interruptStatus
			currentController->hba->interruptStatus = interruptStatus;
			terminalPrintString("msiack", 6);
			for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
				if (
					interruptStatus & ((uint32_t)1 << i) &&
					currentController->devices[i]
				) {
					ahciDeviceMsiHandler(currentController->devices[i]);
				}
			}
		}
		currentController = currentController->next;
	}
	acknowledgeLocalApicInterrupt();
}

bool initializeAHCI(PCIeFunction *pcieFunction) {
	terminalPrintString(initAhciStr, strlen(initAhciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Verify the MSI64 capability
	terminalPrintSpaces4(4);
	terminalPrintString(checkMsiStr, strlen(checkMsiStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!pcieFunction->msi) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
	}
	if (pcieFunction->msi->bit64Capable) {
		PCIeMSI64Capability *msi64 = (PCIeMSI64Capability*)pcieFunction->msi;
		msi64->messageAddress = LOCAL_APIC_DEFAULT_ADDRESS;
		msi64->data = availableInterrupt;
		msi64->enable = 1;
	} else {
		pcieFunction->msi->messageAddress = LOCAL_APIC_DEFAULT_ADDRESS;
		pcieFunction->msi->data = availableInterrupt;
		pcieFunction->msi->enable = 1;
	}
	installIdt64Entry(availableInterrupt, &ahciMsiHandlerWrapper);
	++availableInterrupt;
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Map the HBA control registers to kernel address space
	terminalPrintSpaces4(4);
	terminalPrintString(mappingHbaStr, strlen(mappingHbaStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	AHCIController *currentController;
	if (!ahciControllers) {
		currentController = ahciControllers = kernelMalloc(sizeof(AHCIController));
	} else {
		currentController = ahciControllers;
		while (currentController->next) {
			currentController = currentController->next;
		}
		currentController->next = kernelMalloc(sizeof(AHCIController));
		currentController = currentController->next;
	}
	PCIeType0Header *ahciHeader = (PCIeType0Header*)(pcieFunction->configurationSpace);
	PageRequestResult requestResult = requestVirtualPages(
		2,
		MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS | MEMORY_REQUEST_CACHE_DISABLE
	);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 2 ||
		!mapVirtualPages(requestResult.address, (void*)(uint64_t)ahciHeader->bar5, 2, MEMORY_REQUEST_CACHE_DISABLE)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		return false;
	}
	currentController->hba = requestResult.address;
	currentController->deviceCount = 0;
	for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
		currentController->devices[i] = NULL;
	}
	currentController->next = NULL;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Switch to AHCI mode if hostCapabilities.ahciOnly is not set
	terminalPrintSpaces4(4);
	terminalPrintString(ahciSwitchStr, strlen(ahciSwitchStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!currentController->hba->hostCapabilities.ahciOnly) {
		currentController->hba->globalHostControl |= AHCI_MODE;
	}
	currentController->hba->globalHostControl |= AHCI_GHC_INTERRUPT_ENABLE;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Enumerate and configure all the implemented ports
	terminalPrintSpaces4(4);
	terminalPrintString(probingPortsStr, strlen(probingPortsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	uint32_t portsImplemented = currentController->hba->portsImplemented;
	AHCIDevice *bootCd;
	for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
		if (
			(portsImplemented & 1) &&
			currentController->hba->ports[i].sataStatus.deviceDetection == AHCI_PORT_DEVICE_PRESENT &&
			currentController->hba->ports[i].sataStatus.powerMgmt == AHCI_PORT_ACTIVE
		) {
			AHCIDevice *ahciDevice = kernelMalloc(sizeof(AHCIDevice));
			ahciDevice->portNumber = i;
			ahciDevice->port = &currentController->hba->ports[i];
			ahciDevice->runningCommands = 0;
			ahciDevice->commandHeaders = NULL;
			ahciDevice->fisBase = NULL;
			for (size_t j = 0; j < AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader); ++j) {
				ahciDevice->commandTables[j] = NULL;
				ahciDevice->commandCallbacks[i] = NULL;
			}
			currentController->devices[currentController->deviceCount] = ahciDevice;
			switch (ahciDevice->port->signature) {
				case AHCI_PORT_SIGNATURE_SATA:
					ahciDevice->type = AHCI_PORT_TYPE_SATA;
					break;
				case AHCI_PORT_SIGNATURE_SATAPI:
					ahciDevice->type = AHCI_PORT_TYPE_SATAPI;
					bootCd = ahciDevice;
					break;
				default:
					ahciDevice->type = AHCI_PORT_TYPE_NONE;
					break;
			}
			if (
				ahciDevice->type != AHCI_PORT_TYPE_NONE &&
				!configureAhciDevice(currentController, ahciDevice)
			) {
				terminalPrintString(failedStr, strlen(failedStr));
				terminalPrintChar('\n');
				return false;
			}
			++currentController->deviceCount;
		}
		portsImplemented >>= 1;
	}
	terminalPrintSpaces4();
	terminalPrintString(configuredStr, strlen(configuredStr));
	terminalPrintChar('\n');

	terminalPrintHex(&currentController->hba->interruptStatus, sizeof(currentController->hba->interruptStatus));
	requestResult = requestVirtualPages(1, MEMORY_REQUEST_CONTIGUOUS | MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE | MEMORY_REQUEST_CACHE_DISABLE);
	if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
		return false;
	}
	memset(requestResult.address, 0, pageSize);
	if (!ahciAtapiRead(currentController, bootCd, 36, 1, requestResult.address, &callback)) {
		return false;
	}
	terminalPrintChar('[');
	terminalPrintString(requestResult.address, 4);
	terminalPrintChar(']');
	terminalPrintHex(requestResult.address, 4);
	terminalPrintHex(&bootCd->port->interruptStatus, 4);
	terminalPrintHex(&currentController->hba->interruptStatus, 4);
	if (!ahciAtapiRead(currentController, bootCd, 16, 1, requestResult.address, &callback2)) {
		return false;
	}
	terminalPrintChar('[');
	terminalPrintString(requestResult.address, 4);
	terminalPrintChar(']');
	terminalPrintHex(requestResult.address, 4);
	terminalPrintHex(&bootCd->port->interruptStatus, 4);
	terminalPrintHex(&currentController->hba->interruptStatus, 4);

	terminalPrintString(initAhciCompleteStr, strlen(initAhciCompleteStr));
	return true;
}

bool ahciAtapiRead(
	AHCIController *controller,
	AHCIDevice *device,
	size_t startSector,
	size_t sectorCount,
	void *buffer,
	void (*callback)()
) {
	// Make sure buffer is mapped to a physical page and page boundary aligned
	PML4CrawlResult crawl = crawlPageTables(buffer);
	if (crawl.physicalTables[0] == INVALID_ADDRESS || crawl.indexes[0] != 0) {
		return false;
	}

	// Busy wait till port is busy
	size_t spin = 0;
	while (
		(spin < SIZE_MAX / 2) &&
		(device->port->taskFileData & (AHCI_DEVICE_BUSY | AHCI_DEVICE_DRQ))
	) {
		++spin;
	}
	if (spin == SIZE_MAX / 2) {
		return false;
	}
	terminalPrintString("read", 4);

	// Clear pending interrupts
	device->port->interruptStatus = ~((uint32_t)0);
	size_t freeSlot = findAhciFreeCommandSlot(device);
	if (freeSlot == SIZE_MAX) {
		return false;
	}

	// Refer section 5.5.1 and 5.6.2.4 of AHCI specification https://www.intel.com.au/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
	device->commandHeaders[freeSlot].prdtLength = 1;
	device->commandHeaders[freeSlot].commandFisLength = sizeof(AHCIFISRegisterH2D) / sizeof(uint32_t);
	device->commandHeaders[freeSlot].atapi = 1;
	device->commandHeaders[freeSlot].write = 0;

	memset(device->commandTables[freeSlot], 0, sizeof(AHCICommandTable) + (device->commandHeaders[freeSlot].prdtLength - 1) * sizeof(AHCIPRDTEntry));
	terminalPrintString("mems", 4);
	device->commandTables[freeSlot]->prdtEntries[0].dataBase = (uint32_t)(uint64_t)crawl.physicalTables[0];
	if (controller->hba->hostCapabilities.bit64Addressing) {
		device->commandTables[freeSlot]->prdtEntries[0].dataBaseUpper = (uint32_t)((uint64_t)crawl.physicalTables[0] >> 32);
	}
	device->commandTables[freeSlot]->prdtEntries[0].byteCount = 2048 - 1;
	device->commandTables[freeSlot]->prdtEntries[0].interruptOnCompletion = 1;

	AHCIFISRegisterH2D *commandFis = (AHCIFISRegisterH2D*)&device->commandTables[freeSlot]->commandFIS;
	memset(commandFis, 0, sizeof(AHCIFISRegisterH2D));
	commandFis->fisType = AHCI_FIS_TYPE_REG_H2D;
	commandFis->commandControl = 1;
	commandFis->command = AHCI_COMMAND_ATAPI_PACKET;

	// Set DMA bit and DMA direction (1 = device to host) bit in the features
	// Refer section 7.18.3 and 7.18.4 in https://people.freebsd.org/~imp/asiabsdcon2015/works/d2161r5-ATAATAPI_Command_Set_-_3.pdf
	commandFis->featureLow = 5;

	// Fill the ACMD block with SCSI read(12) command
	// The LBA and sector count fields are in big-endian
	// Refer 3.17 READ(12) section in https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
	device->commandTables[freeSlot]->atapiCommand[0] = ATAPI_READTOC;
	device->commandTables[freeSlot]->atapiCommand[1] = 0;
	device->commandTables[freeSlot]->atapiCommand[2] = (uint8_t)((startSector >> 24) & 0xff);
	device->commandTables[freeSlot]->atapiCommand[3] = (uint8_t)((startSector >> 16) & 0xff);
	device->commandTables[freeSlot]->atapiCommand[4] = (uint8_t)((startSector >> 8) & 0xff);
	device->commandTables[freeSlot]->atapiCommand[5] = (uint8_t)(startSector & 0xff);
	device->commandTables[freeSlot]->atapiCommand[6] = (uint8_t)((sectorCount >> 24) & 0xff);
	device->commandTables[freeSlot]->atapiCommand[7] = (uint8_t)((sectorCount >> 16) & 0xff);
	device->commandTables[freeSlot]->atapiCommand[8] = (uint8_t)((sectorCount >> 8) & 0xff);
	device->commandTables[freeSlot]->atapiCommand[9] = (uint8_t)(sectorCount & 0xff);
	device->commandTables[freeSlot]->atapiCommand[10] = 0;
	device->commandTables[freeSlot]->atapiCommand[11] = 0;
	device->commandTables[freeSlot]->atapiCommand[12] = 0;
	device->commandTables[freeSlot]->atapiCommand[13] = 0;
	device->commandTables[freeSlot]->atapiCommand[14] = 0;
	device->commandTables[freeSlot]->atapiCommand[15] = 0;

	terminalPrintDecimal(device->port->commandIssue);
	device->port->commandIssue = 1 << freeSlot;
	device->runningCommands = 1 << freeSlot;
	device->commandCallbacks[freeSlot] = callback;
	terminalPrintDecimal(device->port->commandIssue);
	terminalPrintString("issu", 4);

	spin = 0;
	while (spin < 1000000) {
		// FIXME: enable AHCI interrupts
		if (device->port->interruptStatus & AHCI_TASK_FILE_ERROR) {
			terminalPrintString("iser", 4);
			return false;
		}
		++spin;
	}
	terminalPrintDecimal(device->port->commandIssue);
	terminalPrintString("rddn", 4);

	return true;
}

bool configureAhciDevice(AHCIController *controller, AHCIDevice *device) {
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(configuringStr, strlen(configuringStr));
	terminalPrintDecimal(device->portNumber);
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	stopAhciCommand(device);

	// Request a page where the command list (1024 bytes) and received FISes (256 bytes) can be placed
	// Get the physical address of this page and put it in the commandListBase and fisBase
	PageRequestResult requestResult = requestVirtualPages(
		1,
		MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS | MEMORY_REQUEST_ALLOCATE_PHYSICAL_PAGE | MEMORY_REQUEST_CACHE_DISABLE
	);
	if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != 1) {
		return false;
	}
	PML4CrawlResult crawlResult = crawlPageTables(requestResult.address);
	uint64_t phyAddr = (uint64_t)crawlResult.physicalTables[0];
	uint64_t virAddr = (uint64_t)requestResult.address;
	memset(requestResult.address, 0, pageSize);
	device->commandHeaders = (void*)virAddr;
	device->port->commandListBase = (uint32_t)phyAddr;
	if (controller->hba->hostCapabilities.bit64Addressing) {
		device->port->commandListBaseUpper = (uint32_t)(phyAddr >> 32);
	}
	phyAddr += AHCI_COMMAND_LIST_SIZE;
	virAddr += AHCI_COMMAND_LIST_SIZE;
	device->fisBase = (void*)virAddr;
	device->port->fisBase = (uint32_t)phyAddr;
	if (controller->hba->hostCapabilities.bit64Addressing) {
		device->port->fisBaseUpper = (uint32_t)(phyAddr >> 32);
	}

	// There are AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader) i.e. 32 command tables
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
	crawlResult = crawlPageTables(requestResult.address);
	phyAddr = (uint64_t)crawlResult.physicalTables[0];
	virAddr = (uint64_t)requestResult.address;
	memset(requestResult.address, 0, 2 * pageSize);
	const size_t commandTableSize = 64 + 16 + 48 + 16 * 8;
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader); ++i) {
		device->commandHeaders[i].prdtLength = 8;
		device->commandTables[i] = (void*)virAddr;
		device->commandHeaders[i].commandTableBase = (uint32_t)phyAddr;
		if (controller->hba->hostCapabilities.bit64Addressing) {
			device->commandHeaders[i].commandTableBaseUpper = (uint32_t)(phyAddr >> 32);
		}
		phyAddr += commandTableSize;
		virAddr += commandTableSize;
	}
	device->port->interruptEnable = (AHCI_TASK_FILE_ERROR | AHCI_REGISTER_D2H);

	startAhciCommand(device);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	return true;
}

void startAhciCommand(AHCIDevice *device) {
	while (device->port->commandStatus.commandListRunning) {
		// Busy wait till the HBA is running some command
	}
	device->port->commandStatus.fisReceiveEnable = 1;
	device->port->commandStatus.start = 1;
}

void stopAhciCommand(AHCIDevice *device) {
	device->port->commandStatus.start = 0;
	device->port->commandStatus.fisReceiveEnable = 0;
	while (
		device->port->commandStatus.fisReceiveRunning ||
		device->port->commandStatus.commandListRunning
	) {
		// Busy wait till the HBA is receiving FIS or running some command
	}
}

size_t findAhciFreeCommandSlot(AHCIDevice *device) {
	uint32_t slots = device->port->commandIssue | device->port->sataActive;
	for (size_t i = 0; i < AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader); ++i) {
		if ((slots & 1) == 0) {
			return i;
		}
		slots >>= 1;
	}
	return SIZE_MAX;
}
