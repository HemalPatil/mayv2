#include <drivers/storage/ahci/satapidevice.h>

AHCI::SatapiDevice::SatapiDevice(Controller *controller) : Device(controller) {
	this->type = AHCI_PORT_TYPE_SATAPI;
}
