#include <drivers/storage/ahci/satapidevice.h>
#include <string.h>

AHCI::SatapiDevice::SatapiDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Satapi;
}

bool AHCI::SatapiDevice::read(size_t startSector, size_t sectorCount, void *buffer, const CommandCallback &callback) {
	size_t freeSlot = SIZE_MAX;
	if (!this->setupRead(sectorCount, buffer, freeSlot)) {
		return false;
	}

	this->commandHeaders[freeSlot].atapi = 1;

	// Setup the command FIS
	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
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

	this->issueCommand(freeSlot, callback);
	return true;
}