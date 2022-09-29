#include <commonstrings.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <drivers/storage/ahci/satadevice.h>
#include <drivers/storage/ahci/satapidevice.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const char* const mappingHbaStr = "Mapping HBA control registers to kernel address space";
static const char* const ahciSwitchStr = "Switching to AHCI mode";
static const char* const probingPortsStr = "Enumerating and configuring ports";
static const char* const configuredStr = "Ports configured";

AHCI::Controller::Controller() {
	this->hba = NULL;
	this->deviceCount = 0;
	for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
		this->devices[i] = NULL;
	}
	this->next = NULL;
}

bool AHCI::Controller::initialize(PCIeFunction *pcieFunction) {
	// Map the HBA control registers to kernel address space
	terminalPrintSpaces4();
	terminalPrintString(mappingHbaStr, strlen(mappingHbaStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PCIeType0Header *ahciHeader = (PCIeType0Header*)(pcieFunction->configurationSpace);
	PageRequestResult requestResult = requestVirtualPages(
		2,
		MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS | MEMORY_REQUEST_CACHE_DISABLE
	);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 2 ||
		!mapVirtualPages(requestResult.address, (void*)(uint64_t)ahciHeader->bar5, 2, MEMORY_REQUEST_CACHE_DISABLE)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		return false;
	}
	this->hba = (HostBusAdapter*)requestResult.address;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Switch to AHCI mode if hostCapabilities.ahciOnly is not set
	terminalPrintSpaces4();
	terminalPrintString(ahciSwitchStr, strlen(ahciSwitchStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!this->hba->hostCapabilities.ahciOnly) {
		this->hba->globalHostControl = this->hba->globalHostControl | AHCI_MODE;
	}
	this->hba->globalHostControl = this->hba->globalHostControl | AHCI_GHC_INTERRUPT_ENABLE;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Enumerate and configure all the implemented ports
	terminalPrintSpaces4();
	terminalPrintString(probingPortsStr, strlen(probingPortsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');
	uint32_t portsImplemented = this->hba->portsImplemented;
	for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
		if (
			(portsImplemented & 1) &&
			this->hba->ports[i].sataStatus.deviceDetection == AHCI_PORT_DEVICE_PRESENT &&
			this->hba->ports[i].sataStatus.powerMgmt == AHCI_PORT_ACTIVE
		) {
			Device *ahciDevice = NULL;
			switch (this->hba->ports[i].signature) {
				case AHCI_PORT_SIGNATURE_SATA:
					ahciDevice = new SataDevice(this);
					break;
				case AHCI_PORT_SIGNATURE_SATAPI:
					ahciDevice = new SatapiDevice(this);
					break;
				default:
					break;
			}
			if (ahciDevice) {
				ahciDevice->portNumber = i;
				ahciDevice->port = &this->hba->ports[i];
				this->devices[this->deviceCount] = ahciDevice;
				if (
					ahciDevice->type != AHCI_PORT_TYPE_NONE &&
					!(
						ahciDevice->initialize() &&
						ahciDevice->identify()
					)
				) {
					terminalPrintString(failedStr, strlen(failedStr));
					terminalPrintChar('\n');
					return false;
				}
			}
			++this->deviceCount;
		}
		portsImplemented >>= 1;
	}
	terminalPrintSpaces4();
	terminalPrintString(configuredStr, strlen(configuredStr));
	terminalPrintChar('\n');
	return true;
}
