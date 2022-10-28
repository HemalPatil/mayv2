#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/blockdevice.h>
#include <functional>

class AHCI::Device : public Storage::BlockDevice {
	public:
		enum Type {
			None = 0,
			Sata,
			Satapi
		};

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
		Type type;

		void issueCommand(size_t freeSlot, const CommandCallback &callback);
		bool setupRead(size_t blockCount, void *buffer, size_t &freeSlot);

	public:
		Device(Controller *controller, size_t portNumber);
		size_t findFreeCommandSlot() const;
		Type getType() const;
		bool identify();
		bool initialize();
		void msiHandler();
		// TODO: implement destructor that releases the virtual pages used for command list and tables

	friend class Controller;
};
