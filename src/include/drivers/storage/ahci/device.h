#pragma once

#include <drivers/storage/ahci.h>

class AHCIDevice {
	public:
		uint8_t type;
		uint8_t portNumber;
		volatile AHCIPort *port;
		AHCICommandHeader *commandHeaders;
		void *fisBase;
		AHCICommandTable *commandTables[AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader)];
		uint32_t runningCommands;
		void (*commandCallbacks[AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader)])();
		AHCIIdentifyDeviceData *info;

		AHCIDevice(AHCIController *controller);
};
