#include <drivers/storage/ahci/satadevice.h>
#include <string.h>

AHCI::SataDevice::SataDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Sata;
}

bool AHCI::SataDevice::read(size_t startSector, size_t sectorCount, void *buffer, const CommandCallback &callback) {
	size_t freeSlot = SIZE_MAX;
	if (!this->setupRead(sectorCount, buffer, freeSlot)) {
		return false;
	}

	this->commandHeaders[freeSlot].atapi = 0;

	// Setup the command FIS
	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
	commandFis->command = AHCI_COMMAND_READ_DMA_EX;
	commandFis->lba0 = (uint8_t)startSector;
	commandFis->lba1 = (uint8_t)(startSector >> 8);
	commandFis->lba2 = (uint8_t)(startSector >> 16);
	commandFis->lba3 = (uint8_t)(startSector >> 24);
	commandFis->lba4 = (uint8_t)(startSector >> 32);
	commandFis->lba5 = (uint8_t)(startSector >> 40);
	commandFis->device = 1 << 6;	// 48-bit LBA mode
	commandFis->countLow = (uint8_t)(sectorCount & 0xff);
	commandFis->countHigh = (uint8_t)((sectorCount & 0xff00) >> 8);

	this->issueCommand(freeSlot, callback);
	return true;
}
