#include <drivers/storage/ahci/satapidevice.h>

AHCI::SatapiDevice::SatapiDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Satapi;
}

bool AHCI::SatapiDevice::read(size_t startSector, size_t count, void *buffer) {
	// TODO: complete read implementation
	return true;
}
