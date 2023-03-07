#pragma once

#include <acpi.h>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <infotable.h>
#include <pml4t.h>
#include <terminal.h>
#include <vector>

#define GIB_1 (1024 * 1024 * 1024UL)
#define KIB_4 (4 * 1024UL)
#define MIB_2 (2 * 1024 * 1024UL)

#define APU_BOOTLOADER_PADDING 32
#define APU_BOOTLOADER_ORIGIN 0x8000
#define INVALID_ADDRESS ((void*) 0x8000000000000000)
#define KERNEL_ORIGIN 0xffffffff80000000
#define L32_IDENTITY_MAP_SIZE 32
#define L32K64_SCRATCH_BASE 0x80000
#define L32K64_SCRATCH_LENGTH 0x10000
#define PHY_MEM_BUDDY_MAX_ORDER 10

namespace Kernel {
	extern bool debug;

	typedef void(*GlobalConstructor)();

	extern InfoTable infoTable;

	extern "C" [[noreturn]] void panic();

	// Flush the virtual->physical address cache by reloading cr3 register
	extern "C" void flushTLB(void *newPml4Root);

	// Halts the system and returns if execution resumed due to any interrupt
	extern "C" void haltSystem();

	// Disables interrupts, halts the systems, and never returns
	extern "C" [[noreturn]] void hangSystem();

	extern "C" void prepareApuInfoTable(ApuInfoTable *apuInfoTable, uint64_t pml4tPhysicalAddress);

	enum IRQ : uint8_t {
		Keyboard = 1,
		Timer = 3
	};

	namespace Scheduler {
		enum TimerType {
			None = 0,
			HPET = 1
		};

		extern TimerType timerUsed;

		void queueEvent(std::coroutine_handle<> event);
		bool start();
		void timerLoop();
	}

	namespace Memory {
		extern const size_t pageSize;
		extern const size_t pageSizeShift;

		enum RequestType : uint32_t {
			Kernel = 1,
			AllocatePhysical = 2,
			CacheDisable = 4,
			PhysicalContiguous = 8,
			VirtualContiguous = 16,
			Executable = 32,
			Writable = 64,
		};

		enum MarkPageType {
			Free = 0,
			Used
		};

		struct [[nodiscard]] PageRequestResult {
			void* address = INVALID_ADDRESS;
			size_t allocatedCount = 0;
		};

		namespace Physical {
			class [[nodiscard]] BuddyBitmapIndex {
				public:
					size_t byte = SIZE_MAX;
					size_t bit = SIZE_MAX;

					BuddyBitmapIndex(void *address, size_t order);
			};

			extern uint8_t* buddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER];
			extern size_t buddyBitmapSizes[PHY_MEM_BUDDY_MAX_ORDER];
			extern uint64_t buddyMasks[PHY_MEM_BUDDY_MAX_ORDER];
			extern size_t buddySizes[PHY_MEM_BUDDY_MAX_ORDER];

			[[nodiscard]] bool areBuddiesOfType(void* address, size_t order, size_t count, MarkPageType type);
			[[nodiscard]] bool initialize(
				void* usablePhyMemStart,
				size_t kernelSize,
				size_t &phyMemBuddyPagesCount
			);
			void listMapEntries();
			void listUsedBuddies(size_t order);
			void markPages(void* address, size_t count, MarkPageType type);
			[[nodiscard]] PageRequestResult requestPages(size_t count, uint32_t flags);
		}

		namespace Virtual {
			class [[nodiscard]] CrawlResult {
				public:
					size_t indexes[5];
					bool isCanonical;
					PML4E* physicalTables[5];
					PML4E* tables[5];
					bool cached[5];

					CrawlResult(void* virtualAddress);
			};

			struct [[nodiscard]] AddressSpaceNode {
				bool available = false;
				void *base = INVALID_ADDRESS;
				size_t pageCount = 0;
			};

			// TODO: Should perhaps use std::list as the container instead of std::vector
			using AddressSpaceList = std::vector<Kernel::Memory::Virtual::AddressSpaceNode>;

			extern AddressSpaceList generalAddressSpaceList;
			extern AddressSpaceList kernelAddressSpaceList;

			void displayCrawlPageTablesResult(void *virtualAddress);
			[[nodiscard]] bool freePages(void *virtualAddress, size_t count, uint32_t flags);
			[[nodiscard]] bool initialize(
				void *usableKernelSpaceStart,
				size_t phyMemBuddyPagesCount,
				GlobalConstructor (&globalCtors)[]
			);
			[[nodiscard]] bool isCanonical(void *address);
			[[nodiscard]] bool mapPages(void *virtualAddress, void *physicalAddress, size_t count, uint32_t flags);
			[[nodiscard]] PageRequestResult requestPages(size_t count, uint32_t flags);
			void showAddressSpaceList(bool kernelList = true);
			[[nodiscard]] bool unmapPages(void *virtualAddress, size_t count, bool freePhysicalPage);
		}

		namespace Heap {
			extern const size_t newRegionSize;
			extern const size_t minBlockSize;
			extern const size_t entryTableSize;

			enum Signature : uint32_t {
				// Represent the strings "FREE" and "USED"
				Free = 0x45455246,
				Used = 0x44455355
			};

			struct Entry {
				Signature signature;
				uint32_t size;
			} __attribute__((packed));

			struct Header {
				size_t entryCount;
				void **entryTable;
				// Entry *latestEntrySearched;
				size_t remaining;
				size_t size;
				struct Header *next;
				struct Header *previous;
			};

			[[nodiscard]] bool create(void *newHeapAddress, void **entryTable);
			void free(void *address);
			void listRegions(bool forwardDirection = true);
			void* allocate(size_t count);
		}
	}
}
