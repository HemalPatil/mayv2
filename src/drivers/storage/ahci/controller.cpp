#include <commonstrings.h>
#include <cstring>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <drivers/storage/ahci/satadevice.h>
#include <drivers/storage/ahci/satapidevice.h>
#include <kernel.h>
#include <terminal.h>

static const char* const mappingHbaStr = "Mapping HBA control registers to kernel address space";
static const char* const ahciSwitchStr = "Switching to AHCI mode";
static const char* const probingPortsStr = "Enumerating and configuring ports";
static const char* const configuredStr = "Ports configured";
static const char* const portStr = "Port ";

Async::Thenable<bool> AHCI::Controller::initialize(const PCIe::Function &pcieFunction) {
	using namespace Kernel::Memory;

	// Map the HBA control registers to kernel address space
	terminalPrintSpaces4();
	terminalPrintString(mappingHbaStr, strlen(mappingHbaStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	PCIe::Type0Header *ahciHeader = (PCIe::Type0Header*)(pcieFunction.configurationSpace);
	PageRequestResult requestResult = Virtual::requestPages(
		2,
		(
			RequestType::Kernel |
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous |
			RequestType::CacheDisable
		)
	);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 2 ||
		!Virtual::mapPages(
			requestResult.address,
			(void*)(uint64_t)ahciHeader->bar5,
			2,
			RequestType::CacheDisable
		)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		co_return false;
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
	for (size_t portNumber = 0; portNumber < AHCI_PORT_COUNT; ++portNumber) {
		if (
			(portsImplemented & 1) &&
			this->hba->ports[portNumber].sataStatus.deviceDetection == AHCI_PORT_DEVICE_PRESENT &&
			this->hba->ports[portNumber].sataStatus.powerMgmt == AHCI_PORT_ACTIVE
		) {
			std::shared_ptr<Device> ahciDevice = nullptr;
			switch (this->hba->ports[portNumber].signature) {
				case PortSignature::SATA:
					ahciDevice = std::make_shared<SataDevice>(this, portNumber);
					break;
				case PortSignature::SATAPI:
					ahciDevice = std::make_shared<SatapiDevice>(this, portNumber);
					break;
				default:
					break;
			}
			if (ahciDevice) {
				this->devices.push_back(ahciDevice);
				terminalPrintSpaces4();
				terminalPrintSpaces4();
				terminalPrintString(portStr, strlen(portStr));
				terminalPrintDecimal(ahciDevice->portNumber);
				terminalPrintChar(':');
				terminalPrintChar('\n');
				if (!ahciDevice->initialize() || !(co_await ahciDevice->identify())) {
					terminalPrintString(failedStr, strlen(failedStr));
					terminalPrintChar('\n');
					co_return false;
				}
			}
		}
		portsImplemented >>= 1;
	}
	terminalPrintSpaces4();
	terminalPrintString(configuredStr, strlen(configuredStr));
	terminalPrintChar('\n');
	co_return true;
}

const std::vector<std::shared_ptr<AHCI::Device>>& AHCI::Controller::getDevices() const {
	return this->devices;
}
