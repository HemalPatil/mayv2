#include <drivers/storage/ahci/satapidevice.h>
#include <string.h>
#include <virtualmemmgmt.h>

AHCI::SatapiDevice::SatapiDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Satapi;
}

bool AHCI::SatapiDevice::read(size_t startSector, size_t sectorCount, void *buffer, const CommandCallback &callback) {
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

	// Refer section 5.5.1 and 5.6.2.4 of AHCI specification https://www.intel.com.au/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
	this->commandHeaders[freeSlot].prdtLength = 1;
	this->commandHeaders[freeSlot].commandFisLength = sizeof(FIS::RegisterH2D) / sizeof(uint32_t);
	this->commandHeaders[freeSlot].atapi = 1;
	this->commandHeaders[freeSlot].write = 0;

	memset(this->commandTables[freeSlot], 0, sizeof(CommandTable) + (this->commandHeaders[freeSlot].prdtLength - 1) * sizeof(PRDTEntry));
	this->commandTables[freeSlot]->prdtEntries[0].dataBase = (uint32_t)bufferPhyAddr;
	if (controller->hba->hostCapabilities.bit64Addressing) {
		this->commandTables[freeSlot]->prdtEntries[0].dataBaseUpper = (uint32_t)(bufferPhyAddr >> 32);
	}
	this->commandTables[freeSlot]->prdtEntries[0].byteCount = this->sectorSize - 1;
	this->commandTables[freeSlot]->prdtEntries[0].interruptOnCompletion = 1;

	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
	memset(commandFis, 0, sizeof(FIS::RegisterH2D));
	commandFis->fisType = AHCI_FIS_TYPE_REG_H2D;
	commandFis->commandControl = 1;
	commandFis->command = AHCI_COMMAND_ATAPI_PACKET;

	// Set DMA bit and DMA direction (1 = device to host) bit in the features
	// Refer section 7.18.3 and 7.18.4 in https://people.freebsd.org/~imp/asiabsdcon2015/works/d2161r5-ATAATAPI_Command_Set_-_3.pdf
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
