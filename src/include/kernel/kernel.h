#pragma once

#include <acpi.h>
#include <cstddef>
#include <cstdint>
#include <infotable.h>
#include <pml4t.h>
#include <vector>

#define GIB_1 (uint64_t)1024 * 1024 * 1024
#define KIB_4 4 * 1024
#define MIB_2 2 * 1024 * 1024

#define INVALID_ADDRESS ((void*) 0x8000000000000000)
#define KERNEL_LOWERHALF_ORIGIN 0x80000000
#define KERNEL_HIGHERHALF_ORIGIN 0xffffffff80000000
#define L32_IDENTITY_MAP_SIZE 32
#define L32K64_SCRATCH_BASE 0x80000
#define L32K64_SCRATCH_LENGTH 0x10000
#define PHY_MEM_BUDDY_MAX_ORDER 10

namespace Kernel {
	extern bool debug;

	typedef void(*GlobalConstructor)();

	// kernelMain is not exposed to other files deliberately

	extern InfoTable *infoTable;

	extern "C" [[noreturn]] void panic();

	// Flush the virtual->physical address cache by reloading cr3 register
	extern "C" void flushTLB(void *newPml4Root);

	// Halts the system and returns if execution resumed due to any interrupt
	extern "C" void haltSystem();

	// Disables interrupts, halts the systems, and never returns
	extern "C" [[noreturn]] void hangSystem();

	namespace Memory {
		extern const size_t pageSize;
		extern const size_t pageSizeShift;

		enum RequestType : uint32_t {
			Contiguous = 1,
			Kernel = 2,
			AllocatePhysical = 4,
			CacheDisable = 8
		};

		enum MarkPageType {
			Free = 0,
			Used
		};

		struct PageRequestResult {
			void* address = INVALID_ADDRESS;
			size_t allocatedCount = 0;
		};

		namespace Physical {
			struct BuddyBitmapIndex {
				size_t byte = SIZE_MAX;
				size_t bit = SIZE_MAX;
			};

			extern ACPI3Entry *map;
			extern uint8_t* buddyBitmaps[PHY_MEM_BUDDY_MAX_ORDER];
			extern size_t buddyBitmapSizes[PHY_MEM_BUDDY_MAX_ORDER];
			extern uint64_t buddyMasks[PHY_MEM_BUDDY_MAX_ORDER];
			extern size_t buddySizes[PHY_MEM_BUDDY_MAX_ORDER];

			bool areBuddiesOfType(void* address, size_t order, size_t count, MarkPageType type);
			BuddyBitmapIndex getBuddyBitmapIndex(void* address, size_t order);
			bool initialize(
				void* usablePhyMemStart,
				size_t kernelLowerHalfSize,
				size_t kernelHigherHalfSize,
				size_t &phyMemBuddyPagesCount
			);
			void initMap();
			void initSize();
			void initUsableSize();
			void listMapEntries();
			void listUsedBuddies(size_t order);
			void markPages(void* address, size_t count, MarkPageType type);
			PageRequestResult requestPages(size_t count, uint32_t flags);
		}

		namespace Virtual {
			class CrawlResult {
				public:
					size_t indexes[5];
					bool isCanonical;
					PML4E* physicalTables[5];
					PML4E* tables[5];
					bool cached[5];

					CrawlResult(void* virtualAddress);
			};

			struct AddressSpaceNode {
				bool available = false;
				void *base = INVALID_ADDRESS;
				size_t pageCount = 0;
			};

			using AddressSpaceList = std::vector<Kernel::Memory::Virtual::AddressSpaceNode>;

			extern AddressSpaceList generalAddressSpaceList;
			extern AddressSpaceList kernelAddressSpaceList;

			void displayCrawlPageTablesResult(void *virtualAddress);
			bool freePages(void *virtualAddress, size_t count, uint8_t flags);
			bool initialize(
				void *usableKernelSpaceStart,
				size_t kernelLowerHalfSize,
				size_t phyMemBuddyPagesCount,
				GlobalConstructor (&globalCtors)[]
			);
			bool isCanonical(void *address);
			bool mapPages(void *virtualAddress, void *physicalAddress, size_t count, uint32_t flags);
			PageRequestResult requestPages(size_t count, uint32_t flags);
			void showAddressSpaceList(bool kernelList = true);
			bool unmapPages(void *virtualAddress, size_t count, bool freePhysicalPage);
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

			extern bool create(void *newHeapAddress, void **entryTable);
			extern void free(void *address);
			extern void listRegions(bool forwardDirection = true);
			extern void* allocate(size_t count);
		}
	}
}
