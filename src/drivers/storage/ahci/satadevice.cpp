#include <drivers/storage/ahci/satadevice.h>

AHCI::SataDevice::SataDevice(Controller *controller) : Device(controller) {
	this->type = AHCI_PORT_TYPE_SATA;
}
