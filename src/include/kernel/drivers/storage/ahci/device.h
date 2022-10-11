#pragma once

#include <drivers/storage/ahci.h>
#include <functional>

class AHCI::Device {
	public:
		enum Type {
			None = 0,
			Sata,
			Satapi
		};
		using CommandCallback = std::function<void()>;

	protected:
		size_t portNumber;
		volatile Port *port;
		CommandHeader *commandHeaders;
		void *fisBase;
		CommandTable *commandTables[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		uint32_t runningCommandsBitmap;
		CommandCallback commandCallbacks[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		IdentifyDeviceData *info;
		Controller *controller;
		size_t sectorSize;
		Type type;

	public:
		Device(Controller *controller, size_t portNumber);
		size_t findFreeCommandSlot();
		bool identify();
		bool initialize();
		void msiHandler();
		virtual bool read(size_t startSector, size_t count, void *buffer, const CommandCallback &callback) = 0;

	friend class Controller;
};
