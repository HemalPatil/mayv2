#include <cstring>
#include <drivers/storage/ahci/satadevice.h>

AHCI::SataDevice::SataDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Sata;
}

Async::Thenable<Storage::Buffer> AHCI::SataDevice::read(size_t startBlock, size_t blockCount) {
	Storage::Buffer buffer;
	size_t freeSlot = SIZE_MAX;
	if (!(buffer = this->setupRead(blockCount, freeSlot))) {
		co_return nullptr;
	}

	this->commandHeaders[freeSlot].atapi = 0;

	// Setup the command FIS
	FIS::RegisterH2D *commandFis = (FIS::RegisterH2D*)&this->commandTables[freeSlot]->commandFIS;
	commandFis->command = AHCI_COMMAND_READ_DMA_EX;
	commandFis->lba0 = (uint8_t)startBlock;
	commandFis->lba1 = (uint8_t)(startBlock >> 8);
	commandFis->lba2 = (uint8_t)(startBlock >> 16);
	commandFis->lba3 = (uint8_t)(startBlock >> 24);
	commandFis->lba4 = (uint8_t)(startBlock >> 32);
	commandFis->lba5 = (uint8_t)(startBlock >> 40);
	commandFis->device = 1 << 6;	// 48-bit LBA mode
	commandFis->countLow = (uint8_t)(blockCount & 0xff);
	commandFis->countHigh = (uint8_t)((blockCount & 0xff00) >> 8);

	if (!co_await Command(this, freeSlot)) {
		co_return nullptr;
	}
	co_return std::move(buffer);
}
