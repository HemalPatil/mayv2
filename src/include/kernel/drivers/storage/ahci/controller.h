#pragma once

#include <drivers/storage/ahci.h>
#include <memory>

class AHCI::Controller {
	private:
		volatile HostBusAdapter *hba;
		std::vector<std::shared_ptr<Device>> devices;

	public:
		const std::vector<std::shared_ptr<Device>>& getDevices() const;
		Async::Thenable<bool> initialize(PCIe::Function &pcieFunction);
		// TODO: implement destructor that releases the virtual pages mapped to the controller's HBA

	friend void ::ahciMsiHandler();
	friend class Device;
	friend class SataDevice;
	friend class SatapiDevice;
};
