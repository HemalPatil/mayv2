#pragma once

#include <drivers/storage/ahci.h>

class AHCI::Device {
	public:
		uint8_t type;
		uint8_t portNumber;
		volatile Port *port;
		CommandHeader *commandHeaders;
		void *fisBase;
		CommandTable *commandTables[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		uint32_t runningCommands;
		void (*commandCallbacks[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)])();
		IdentifyDeviceData *info;
		Controller *controller;

		Device(Controller *controller);
		bool initialize();
		size_t findFreeCommandSlot();
		void msiHandler();
		bool identify();

	friend void ::ahciMsiHandler();
};
