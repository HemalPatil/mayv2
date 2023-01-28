#include <cstring>
#include <kernel.h>
#include <terminal.h>

// The heap region list cannot be a vector unfortunately
// since std::vector itself uses new internally which requires a heap
static Kernel::Memory::Heap::Header *heapList = nullptr;

static bool validHeap(Kernel::Memory::Heap::Header *heap);
static Kernel::Memory::Heap::Entry* nextHeapEntry(
	Kernel::Memory::Heap::Header *heap,
	Kernel::Memory::Heap::Entry *entry
);
static bool validHeapEntry(
	Kernel::Memory::Heap::Header *heap,
	Kernel::Memory::Heap::Entry *entry
);

static const char* const listStr = "List of all heap regions\n";
static const char* const listHeaderStr = "Address              Count                Remaining            Entry table\n";
static const char* const heapNamespaceStr = "Kernel::Memory::Heap::";
static const char* const allocateInvalidCountStr = "allocate invalid request size ";
static const char* const noHeapsStr = "No kernel heap regions created\n";
static const char* const corruptEntryStr = "validHeapEntry corrupt entry ";
static const char* const corruptEntryContStr = " found in heap ";
static const char* const corruptHeapStr = "validHeap corrupt heap ";
static const char* const invalidFreeStr = "free invalid free ";

const size_t Kernel::Memory::Heap::newRegionSize = MIB_2;
const size_t Kernel::Memory::Heap::minBlockSize = 8;
const size_t Kernel::Memory::Heap::entryTableSize = Kernel::Memory::Heap::newRegionSize / (
	sizeof(Kernel::Memory::Heap::Entry) +
	Kernel::Memory::Heap::minBlockSize
);

// Returns a memory chunk from one of the kernel heap regions
// Unsafe to call before at least one heap region is created
void* Kernel::Memory::Heap::allocate(size_t count) {
	if (!heapList) {
		terminalPrintString(noHeapsStr, strlen(noHeapsStr));
		panic();
	}

	// FIXME: All heap regions are currently of size 2MiB so serving more than that is not possible
	if (count == 0 || count >= (newRegionSize - sizeof(Header) - sizeof(Entry))) {
		terminalPrintString(heapNamespaceStr, strlen(heapNamespaceStr));
		terminalPrintString(allocateInvalidCountStr, strlen(allocateInvalidCountStr));
		terminalPrintHex(&count, sizeof(count));
		terminalPrintChar('\n');
		panic();
		return nullptr;
	}

	// Align count to minBlockSize
	if (count % minBlockSize > 0) {
		count += minBlockSize - (count % minBlockSize);
	}

	void *allocatedValue = nullptr;
	Header *currentHeap = heapList;
	while (currentHeap) {
		if (count <= currentHeap->remaining) {
			Entry *entry = (Entry*)((uint64_t)currentHeap + sizeof(Header));
			while (
				entry &&
				validHeapEntry(currentHeap, entry) &&
				(
					entry->signature != Signature::Free ||
					(entry->signature == Signature::Free && entry->size < count)
				)
			) {
				entry = nextHeapEntry(currentHeap, entry);
			}

			// Break the free block only if the newly created free block
			// can occupy at least minBlockSize
			entry->signature = Signature::Used;
			if (entry->size >= count + sizeof(Entry) + minBlockSize) {
				const size_t newFreeEntryRemaining = entry->size - count - sizeof(Entry);
				entry->size = count;
				Entry *newFreeEntry = (Entry*)((uint64_t)entry + entry->size + sizeof(Entry));
				newFreeEntry->signature = Signature::Free;
				newFreeEntry->size = newFreeEntryRemaining;
				currentHeap->remaining -= (count + sizeof(Entry));
			} else {
				currentHeap->remaining -= entry->size;
			}
			allocatedValue = (void*)((uint64_t)entry + sizeof(Entry));
			currentHeap->entryTable[currentHeap->entryCount] = allocatedValue;
			++currentHeap->entryCount;
			break;
		}
		currentHeap = currentHeap->next;
	}

	// Traversed all existing heap regions
	// TODO: Create new heap region and return result of recursive call

	// TODO: remove expensive operation of checking integrity of all heaps for each allocate
	currentHeap = heapList;
	while (currentHeap) {
		if (!validHeap(currentHeap)) {
			return nullptr;
		}
		currentHeap = currentHeap->next;
	}

	return allocatedValue;
}

void Kernel::Memory::Heap::free(void *address) {
	uint64_t addr = (uint64_t)address;
	Header *heap = heapList;
	bool freed = false;
	while (heap) {
		// Ensure the address is within the heap's bounds
		if (
			(addr >= (uint64_t)heap + sizeof(Header) + sizeof(Entry)) &&
			(addr < (uint64_t)heap + heap->size)
		) {
			// Check if this address is allocated in the current heap
			bool entryFound = false;
			size_t i;
			for (i = 0; i < heap->entryCount; ++i) {
				if (heap->entryTable[i] == address) {
					entryFound = true;
					break;
				}
			}
			if (entryFound) {
				--heap->entryCount;
				heap->entryTable[i] = heap->entryTable[heap->entryCount];
				Entry *entry = (Entry*)(addr - sizeof(Entry));
				entry->signature = Signature::Free;
				heap->remaining += entry->size;
				// TODO: defrag the heap
				freed = true;
				break;
			}
		}
		heap = heap->next;
	}

	// TODO: remove expensive operation of checking integrity of all heaps for each free
	Header *currentHeap = heapList;
	while (currentHeap) {
		if (!validHeap(currentHeap)) {
			return;
		}
		currentHeap = currentHeap->next;
	}

	if (freed) {
		return;
	}

	terminalPrintString(heapNamespaceStr, strlen(heapNamespaceStr));
	terminalPrintString(invalidFreeStr, strlen(invalidFreeStr));
	terminalPrintHex(&address, sizeof(address));
	terminalPrintChar('\n');
	panic();
}

static bool validHeap(Kernel::Memory::Heap::Header *heap) {
	using namespace Kernel::Memory::Heap;
	size_t total = sizeof(Header);
	Entry *entry = (Entry*)((uint64_t)heap + sizeof(Header));
	while (entry && validHeapEntry(heap, entry)) {
		total += (sizeof(Entry) + entry->size);
		entry = nextHeapEntry(heap, entry);
	}
	if (total == heap->size) {
		return true;
	}
	terminalPrintString(heapNamespaceStr, strlen(heapNamespaceStr));
	terminalPrintString(corruptHeapStr, strlen(corruptHeapStr));
	terminalPrintHex(&heap, sizeof(heap));
	terminalPrintChar('\n');
	Kernel::panic();
	return false;
}

// Creates a new heap region of size Kernel::Memory::Heap::newRegionSize,
// corresponding entry table of size Kernel::Memory::Heap::entryTableSize,
// and adds it to the heap regions list
// Assumes the newHeapAddress passed is a region in kernel address space
// of total size of heap + entry table
// Returns true if the operation was successful
bool Kernel::Memory::Heap::create(void *newHeapAddress, void **entryTable) {
	PageRequestResult requestResult = Physical::requestPages(newRegionSize / pageSize, RequestType::PhysicalContiguous);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != (newRegionSize/ pageSize) ||
		!Virtual::mapPages(newHeapAddress, requestResult.address, requestResult.allocatedCount, 0)
	) {
		return false;
	}
	memset(newHeapAddress, 0, newRegionSize);
	requestResult = Physical::requestPages(entryTableSize / pageSize, RequestType::PhysicalContiguous);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != (entryTableSize / pageSize) ||
		!Virtual::mapPages(
			(void*)((uint64_t)newHeapAddress + newRegionSize),
			requestResult.address,
			requestResult.allocatedCount,
			0
		)
	) {
		return false;
	}
	Header *currentHeap = nullptr;
	if (!heapList) {
		heapList = (Header*)newHeapAddress;
		heapList->previous = nullptr;
	} else {
		currentHeap = heapList;
		while(currentHeap->next) {
			currentHeap = currentHeap->next;
		}
		currentHeap->next = (Header*)newHeapAddress;
		((Header*)newHeapAddress)->previous = currentHeap;
	}
	currentHeap = (Header*)newHeapAddress;
	currentHeap->entryCount = 0;
	currentHeap->entryTable = entryTable;
	currentHeap->remaining = newRegionSize - sizeof(Header) - sizeof(Entry);
	currentHeap->size = newRegionSize;
	currentHeap->next = nullptr;
	Entry *freeEntry = (Entry*)((uint64_t)currentHeap + sizeof(Header));
	freeEntry->signature = Signature::Free;
	freeEntry->size = currentHeap->remaining;
	return true;
}

static Kernel::Memory::Heap::Entry* nextHeapEntry(
	Kernel::Memory::Heap::Header *heap,
	Kernel::Memory::Heap::Entry *entry
) {
	using namespace Kernel::Memory::Heap;
	uint64_t heapEnd = (uint64_t)heap + heap->size;
	uint64_t nextEntry = (uint64_t)entry + sizeof(Entry) + entry->size;
	if (nextEntry >= heapEnd || (nextEntry + sizeof(Entry) + minBlockSize) > heapEnd) {
		return nullptr;
	}
	return (Entry*)nextEntry;
}

// Returns true only if heap entry signature is Signature::Free or Signature::Used,
// size is less than heap->remaining and heap->size,
// and size is minimum minBlockSize
static bool validHeapEntry(
	Kernel::Memory::Heap::Header *heap,
	Kernel::Memory::Heap::Entry *entry
) {
	using namespace Kernel::Memory::Heap;
	if (
		(entry->signature == Signature::Free || entry->signature == Signature::Used) &&
		entry->size <= heap->remaining &&
		entry->size <= (heap->size - sizeof(Header) - sizeof(Entry)) &&
		entry->size >= minBlockSize
	) {
		return true;
	}
	terminalPrintString(heapNamespaceStr, strlen(heapNamespaceStr));
	terminalPrintString(corruptEntryStr, strlen(corruptEntryStr));
	terminalPrintHex(&entry, sizeof(entry));
	terminalPrintChar('(');
	terminalPrintHex(entry, 8);
	terminalPrintChar(')');
	terminalPrintString(corruptEntryContStr, strlen(corruptEntryContStr));
	terminalPrintHex(&heap, sizeof(heap));
	terminalPrintChar('\n');
	Kernel::Memory::Virtual::displayCrawlPageTablesResult(entry);
	Kernel::panic();
	return false;
}

// Debug helper to list stats about all heap regions
void Kernel::Memory::Heap::listRegions(bool forwardDirection) {
	terminalPrintString(listStr, strlen(listStr));
	terminalPrintString(listHeaderStr, strlen(listHeaderStr));
	Header *currentHeap = heapList;
	if (!forwardDirection) {
		while (currentHeap->next) {
			currentHeap = currentHeap->next;
		}
	}
	while (currentHeap) {
		terminalPrintHex(&currentHeap, sizeof(currentHeap));
		terminalPrintChar(' ');
		terminalPrintHex(&currentHeap->entryCount, sizeof(currentHeap->entryCount));
		terminalPrintChar(' ');
		terminalPrintHex(&currentHeap->remaining, sizeof(currentHeap->remaining));
		terminalPrintChar(' ');
		terminalPrintHex(&currentHeap->entryTable, sizeof(currentHeap->entryTable));
		terminalPrintChar('\n');
		currentHeap = forwardDirection ? currentHeap->next : currentHeap->previous;
	}
}
