#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>

class AHCI::SatapiDevice : public AHCI::Device {
	public:
		SatapiDevice(Controller *controller);
};
