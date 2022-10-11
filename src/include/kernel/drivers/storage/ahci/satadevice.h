#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>

class AHCI::SataDevice : public AHCI::Device {
	public:
		SataDevice(Controller *controller, size_t portNumber);
		bool read(size_t startSector, size_t sectorCount, void *buffer, const CommandCallback &callback) override;
};
