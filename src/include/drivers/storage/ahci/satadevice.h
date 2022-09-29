#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>

class AHCI::SataDevice : public AHCI::Device {
	public:
		SataDevice(Controller *controller);
};
