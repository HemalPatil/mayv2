#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>

class AHCI::SatapiDevice : public AHCI::Device {
	public:
		SatapiDevice(Controller *controller, size_t portNumber);

		// Reads maximum of 32MiB data from an ATA/ATAPI device attached to an AHCI controller.
		// It is assumed that buffer is a valid virtual address mapped to some physical address,
		// contiguous, word boundary aligned, and big enough to handle the entire data requested.
		// Returns false if total byte length of data requested is greater than 32MiB,
		// or buffer is not word boundary aligned, or buffer is not a valid virtual address
		// mapped to a physical address, or there is no command slot currently available.
		bool read(size_t startBlock, size_t blockCount, void *buffer, const CommandCallback &callback) override;
};
