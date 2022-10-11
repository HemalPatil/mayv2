#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>

class AHCI::SatapiDevice : public AHCI::Device {
	public:
		SatapiDevice(Controller *controller, size_t portNumber);
		bool read(size_t startSector, size_t count, void *buffer) override;
};
