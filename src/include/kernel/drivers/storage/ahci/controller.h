#pragma once

#include <drivers/storage/ahci.h>

class AHCI::Controller {
	private:
		volatile HostBusAdapter *hba;
		size_t deviceCount;
		Device *devices[AHCI_PORT_COUNT];

	public:
		Device* getDevice(size_t portNumber) const;
		bool initialize(PCIeFunction *pcieFunction);
		// TODO: implement destructor that releases the virtual pages mapped to the controller's HBA

	friend void ::ahciMsiHandler();
	friend class Device;
	friend class SataDevice;
	friend class SatapiDevice;
};
