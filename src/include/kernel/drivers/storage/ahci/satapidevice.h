#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>

class AHCI::SatapiDevice : public AHCI::Device {
	public:
		SatapiDevice(Controller *controller, size_t portNumber);

		// Reads maximum of 32MiB data from an ATA/ATAPI device attached to an AHCI controller.
		// Returns Storage::Buffer containing the data read
		// Returns nullptr if the read failed or size constraints are not met
		Async::Thenable<std::shared_ptr<Storage::Buffer>> read(size_t startBlock, size_t blockCount) override;
};
