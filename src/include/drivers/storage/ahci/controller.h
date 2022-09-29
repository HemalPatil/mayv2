#pragma once

#include <drivers/storage/ahci.h>

class AHCI::Controller {
	private:
		volatile HostBusAdapter *hba;
		size_t deviceCount;
		Device *devices[AHCI_PORT_COUNT];
		Controller *next;

	public:
		Controller();
		bool initialize(PCIeFunction *pcieFunction);

	friend void ::ahciMsiHandler();
	friend bool initialize(PCIeFunction *pcieFunction);
	friend class Device;
};
