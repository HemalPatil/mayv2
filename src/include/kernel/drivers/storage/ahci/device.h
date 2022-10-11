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

		void issueCommand(size_t freeSlot, const CommandCallback &callback);
		bool setupRead(size_t sectorCount, void *buffer, size_t &freeSlot);

	public:
		Device(Controller *controller, size_t portNumber);
		size_t findFreeCommandSlot() const;
		size_t getSectorSize() const;
		Type getType() const;
		bool identify();
		bool initialize();
		void msiHandler();

		// Reads maximum of 32MiB data from an ATA/ATAPI device attached to an AHCI controller.
		// It is assumed that buffer is a valid virtual address mapped to some physical address,
		// contiguous, word boundary aligned, and big enough to handle the entire data requested.
		// Returns false if total byte length of data requested is greater than 32MiB,
		// or buffer is not word boundary aligned, or buffer is not a valid virtual address
		// mapped to a physical address, or there is no command slot currently available.
		virtual bool read(size_t startSector, size_t sectorCount, void *buffer, const CommandCallback &callback) = 0;

	friend class Controller;
};
