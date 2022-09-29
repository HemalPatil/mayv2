#pragma once

#include <drivers/storage/ahci.h>

class AHCIController {
	public:
		volatile AHCIHostBusAdapter *hba;
		size_t deviceCount;
		AHCIDevice *devices[AHCI_PORT_COUNT];
		AHCIController *next;
};
