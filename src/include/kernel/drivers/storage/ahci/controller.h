#pragma once

#include <drivers/storage/ahci.h>

class AHCI::Controller {
	private:
		volatile HostBusAdapter *hba;
		size_t deviceCount;
		Device *devices[AHCI_PORT_COUNT];

	public:
		Controller *next;
		bool initialize(PCIeFunction *pcieFunction);

	friend void ::ahciMsiHandler();
	friend class Device;
};
