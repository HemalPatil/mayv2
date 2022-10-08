#pragma once

#include <drivers/storage/ahci.h>
#include <functional.cpp>
// #include <memory>

class AHCI::Device {
	private:
		uint8_t portNumber;
		volatile Port *port;
		CommandHeader *commandHeaders;
		void *fisBase;
		CommandTable *commandTables[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		uint32_t runningCommandsBitmap;
		std::function<void()> commandCallbacks[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		IdentifyDeviceData *info;
		Controller *controller;

	protected:
		uint8_t type;

	public:
		Device(Controller *controller);
		bool initialize();
		size_t findFreeCommandSlot();
		void msiHandler();
		bool identify();

	friend void ::ahciMsiHandler();
	friend class Controller;
};
