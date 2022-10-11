#include <drivers/storage/ahci/satapidevice.h>
#include <string.h>
#include <virtualmemmgmt.h>

AHCI::SatapiDevice::SatapiDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Satapi;
}

// Reads maximum of 32MiB data from an ATAPI device attached to an AHCI controller.
// It is assumed that buffer is a valid virtual address mapped to some physical address,
// contiguous, word boundary aligned, and big enough to handle the entire data requested.
// Returns false if total byte length of data requested is greater than 32MiB,
// or buffer is not word boundary aligned, or buffer is not a valid virtual address
// mapped to a physical address, or there is no command slot currently available.
bool AHCI::SatapiDevice::read(size_t startSector, size_t sectorCount, void *buffer, const CommandCallback &callback) {
	// Each command has 8 PRDTs (read AHCI::Device::initialize comments to know why 8 PRDTs)
	// and each PRDT can handle 4MiB i.e. maximum of 32MiB
	const size_t mib4 = 4 * 1024 * 1024UL;
	const size_t mib32 = 8 * mib4;
	const size_t totalBytes = sectorCount * this->sectorSize;
	if (totalBytes > mib32) {
		return false;
	}

	// Make sure buffer is mapped to a physical page and word boundary aligned
	PML4CrawlResult crawlResult(buffer);
	if (crawlResult.physicalTables[0] == INVALID_ADDRESS || crawlResult.indexes[0] % 2 != 0) {
		return false;
	}
	uint64_t bufferPhyAddr = (uint64_t)crawlResult.physicalTables[0] + crawlResult.indexes[0];

	size_t freeSlot = this->findFreeCommandSlot();
	if (freeSlot == SIZE_MAX) {
		return false;
	}

	size_t prdsRequired = totalBytes / mib4;
	if (totalBytes > prdsRequired * mib4) {
		++prdsRequired;
	}

	// Refer section 5.5.1 and 5.6.2.4 of AHCI specification https://www.intel.com.au/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
	this->commandHeaders[freeSlot].prdtLength = prdsRequired;
	this->commandHeaders[freeSlot].commandFisLength = sizeof(FIS::RegisterH2D) / sizeof(uint32_t);
	this->commandHeaders[freeSlot].atapi = 1;
	this->commandHeaders[freeSlot].write = 0;

	// Clear out the command table
	memset(this->commandTables[freeSlot], 0, sizeof(CommandTable) + prdsRequired * sizeof(PRDTEntry));

	// Setup PRDT entries
	// First (prdsRequired - 1) will all have byteCount set to (mib4 - 1) and will not interrupt.
	// The last PRDT entry will have remaining byteCount and must interrupt to signal command completion.
	uint64_t bufferSegmentPhyAddr;
	for (size_t i = 0; i < prdsRequired - 1; ++i) {
		bufferSegmentPhyAddr = bufferPhyAddr + i * mib4;
		this->commandTables[freeSlot]->prdtEntries[i].dataBase = (uint32_t)(bufferSegmentPhyAddr);
		if (controller->hba->hostCapabilities.bit64Addressing) {
			this->commandTables[freeSlot]->prdtEntries[i].dataBaseUpper = (uint32_t)(bufferSegmentPhyAddr >> 32);
		}
		this->commandTables[freeSlot]->prdtEntries[i].byteCount = mib4 - 1;
		this->commandTables[freeSlot]->prdtEntries[i].interruptOnCompletion = 0;
	}
	bufferSegmentPhyAddr = bufferPhyAddr + (prdsRequired - 1) * mib4;
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
	commandFis->command = AHCI_COMMAND_ATAPI_PACKET;

	// Set DMA bit and DMA direction (1 = device to host) bit in the features
	// Technically the DMA direction bit may not need to be set
	// according to word 62 of identify data but it's set here anyway
	// Refer section 7.18.3, 7.18.4, and 7.13.6.1 in https://people.freebsd.org/~imp/asiabsdcon2015/works/d2161r5-ATAATAPI_Command_Set_-_3.pdf
	commandFis->featureLow = 5;

	// Fill the ACMD block with SCSI read(12) command
	// The LBA and sector count fields are in big-endian
	// Refer 3.17 READ(12) section in https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
	this->commandTables[freeSlot]->atapiCommand[0] = ATAPI_READTOC;
	this->commandTables[freeSlot]->atapiCommand[1] = 0;
	this->commandTables[freeSlot]->atapiCommand[2] = (uint8_t)((startSector >> 24) & 0xff);
	this->commandTables[freeSlot]->atapiCommand[3] = (uint8_t)((startSector >> 16) & 0xff);
	this->commandTables[freeSlot]->atapiCommand[4] = (uint8_t)((startSector >> 8) & 0xff);
	this->commandTables[freeSlot]->atapiCommand[5] = (uint8_t)(startSector & 0xff);
	this->commandTables[freeSlot]->atapiCommand[6] = (uint8_t)((sectorCount >> 24) & 0xff);
	this->commandTables[freeSlot]->atapiCommand[7] = (uint8_t)((sectorCount >> 16) & 0xff);
	this->commandTables[freeSlot]->atapiCommand[8] = (uint8_t)((sectorCount >> 8) & 0xff);
	this->commandTables[freeSlot]->atapiCommand[9] = (uint8_t)(sectorCount & 0xff);
	this->commandTables[freeSlot]->atapiCommand[10] = 0;
	this->commandTables[freeSlot]->atapiCommand[11] = 0;
	this->commandTables[freeSlot]->atapiCommand[12] = 0;
	this->commandTables[freeSlot]->atapiCommand[13] = 0;
	this->commandTables[freeSlot]->atapiCommand[14] = 0;
	this->commandTables[freeSlot]->atapiCommand[15] = 0;

	// Issue command
	this->runningCommandsBitmap |= 1 << freeSlot;
	this->commandCallbacks[freeSlot] = callback;
	this->port->commandIssue = 1 << freeSlot;

	return true;
}
