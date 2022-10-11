#include <drivers/storage/ahci/satadevice.h>

AHCI::SataDevice::SataDevice(
	Controller *controller,
	size_t portNumber
) : Device(controller, portNumber) {
	this->type = Type::Sata;
}

bool AHCI::SataDevice::read(size_t startSector, size_t count, void *buffer, const CommandCallback &callback) {
	// TODO: complete read implementation
	return false;
}
