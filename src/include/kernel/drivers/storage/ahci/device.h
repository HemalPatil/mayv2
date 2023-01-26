#pragma once

#include <drivers/storage/ahci.h>
#include <drivers/storage/blockdevice.h>
#include <memory>
#include <promise.h>

class AHCI::Device : public Storage::BlockDevice {
	public:
		enum Type {
			None = 0,
			Sata,
			Satapi
		};

		class [[nodiscard]] Command {
			private:
				Device *device;
				size_t freeSlot;
				std::coroutine_handle<> *awaitingCoroutine;
				bool *result;

			public:
				Command(Device *device, size_t freeSlot);

				Command(const Command&) = delete;
				Command& operator=(const Command&) = delete;

				~Command();

				constexpr bool await_ready() const noexcept;

				void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;

				bool await_resume() const noexcept;

				void setResult(bool result) noexcept;
		};

	protected:
		size_t portNumber;
		volatile Port *port;
		CommandHeader *commandHeaders;
		void *fisBase;
		CommandTable *commandTables[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		uint32_t runningCommandsBitmap;
		Command *commands[AHCI_COMMAND_LIST_SIZE / sizeof(CommandHeader)];
		IdentifyDeviceData *info;
		Controller *controller;
		Type type;

		bool setupRead(size_t blockCount, std::shared_ptr<void> buffer, size_t &freeSlot);

	public:
		Device(Controller *controller, size_t portNumber);
		size_t findFreeCommandSlot() const;
		size_t getPortNumber() const;
		Type getType() const;
		Kernel::Async::Thenable<bool> identify();
		bool initialize();
		void msiHandler();
		// TODO: implement destructor that releases the virtual pages used for command list and tables

	friend class Controller;
};
