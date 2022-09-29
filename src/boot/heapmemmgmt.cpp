#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static HeapEntry* nextHeapEntry(HeapHeader *heap, HeapEntry *heapEntry);

HeapHeader *heapRegionsList = (HeapHeader*)INVALID_ADDRESS;
HeapHeader *latestHeapSearched = (HeapHeader*)INVALID_ADDRESS;

static const char* const initHeapStr = "Initializing dynamic memory management";
static const char* const initHeapCompleteStr = "Dynamic memory management initialized\n\n";
static const char* const creatingHeapHeaderStr = "Creating heap header";
static const char* const movingVirMemListsStr = "Moving virtual address space lists to heap memory";
static const char* const listHeapStr = "List of all kernel heap regions\n";
static const char* const heapHeaderStr = "Heap start           Size                 Count Remaining\n";
static const char* const invalidFreeStr = "\nInvalid kernelFree operation ";

void *operator new(size_t size) {
	return kernelMalloc(size);
}

void *operator new[](size_t size) {
	return kernelMalloc(size);
}

void operator delete(void *memory) {
	kernelFree(memory);
}

void operator delete[](void *memory) {
	kernelFree(memory);
}

// TODO: Do not know the meaning of -Wsized-deallocation but this removes the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void operator delete(void *memory, size_t size) {
	kernelFree(memory);
}

void operator delete[](void *memory, size_t size) {
	kernelFree(memory);
}
#pragma GCC diagnostic pop

bool initializeDynamicMemory() {
	void *ghostPage = kernelAddressSpaceList;
	terminalPrintString(initHeapStr, strlen(initHeapStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Initalize the heap region reserved by virtual memory manager during its initialization
	terminalPrintSpaces4();
	terminalPrintString(creatingHeapHeaderStr, strlen(creatingHeapHeaderStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	heapRegionsList->entryCount = 0;
	heapRegionsList->size = HEAP_NEW_REGION_SIZE;
	heapRegionsList->remaining = heapRegionsList->size - sizeof(HeapHeader) - sizeof(HeapEntry);
	heapRegionsList->next = heapRegionsList->previous = NULL;
	latestHeapSearched = heapRegionsList;
	HeapEntry *freeEntry = (HeapEntry*)((uint64_t)heapRegionsList + sizeof(HeapHeader));
	freeEntry->signature = HEAP_ENTRY_FREE;
	freeEntry->size = heapRegionsList->remaining;
	heapRegionsList->latestEntrySearched = freeEntry;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// Move the virtual address space lists at the end of heap to the heap to make them dynamic
	// and unmap the ghost page containing the lists
	terminalPrintSpaces4();
	terminalPrintString(movingVirMemListsStr, strlen(movingVirMemListsStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	VirtualMemNode *lists[2] = { kernelAddressSpaceList, generalAddressSpaceList };
	VirtualMemNode *newLists[2] = { NULL, NULL };
	for (size_t i = 0; i < 2; ++i) {
		VirtualMemNode *current = new VirtualMemNode();
		memcpy(current, lists[i], sizeof(VirtualMemNode));
		newLists[i] = current;
		VirtualMemNode *newPrevious = current;
		VirtualMemNode *oldCurrent = lists[i]->next;
		while (oldCurrent) {
			current = new VirtualMemNode();
			memcpy(current, oldCurrent, sizeof(VirtualMemNode));
			newPrevious->next = current;
			current->previous = newPrevious;
			newPrevious = current;
			oldCurrent = oldCurrent->next;
		}
	}
	kernelAddressSpaceList = newLists[0];
	generalAddressSpaceList = newLists[1];
	if (!unmapVirtualPages(ghostPage, 1, true)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintString(initHeapCompleteStr, strlen(initHeapCompleteStr));
	return true;
}

// Debug helper to list stats about all heap regions
void listAllHeapRegions() {
	terminalPrintString(listHeapStr, strlen(listHeapStr));
	terminalPrintSpaces4();
	terminalPrintString(heapHeaderStr, strlen(heapHeaderStr));
	HeapHeader *header = heapRegionsList;
	while(header) {
		terminalPrintSpaces4();
		terminalPrintHex(&header, sizeof(header));
		terminalPrintChar(' ');
		terminalPrintHex(&header->size, sizeof(header->size));
		terminalPrintChar(' ');
		terminalPrintDecimal(header->entryCount);
		terminalPrintSpaces4();
		terminalPrintChar(' ');
		terminalPrintHex(&header->remaining, sizeof(header->remaining));
		terminalPrintChar('\n');
		header = header->next;
	}
}

void* kernelMalloc(size_t count) {
	// FIXME: All heap regions are currently of size 2MiB so serving more than that is not possible
	if (count == 0 || count >= HEAP_NEW_REGION_SIZE - sizeof(HeapHeader)) {
		return NULL;
	}

	// Align count to HEAP_MIN_BLOCK_SIZE
	if (count % HEAP_MIN_BLOCK_SIZE) {
		count += HEAP_MIN_BLOCK_SIZE - (count % HEAP_MIN_BLOCK_SIZE);
	}

	HeapHeader *heap = latestHeapSearched;
	if (count > heap->remaining) {
		heap = heapRegionsList;
	}
	while (heap) {
		if (count <= heap->remaining) {
			HeapEntry *heapEntry = heap->latestEntrySearched;
			bool freeEntryFound = true;
			// lastEntrySearched is guaranteed to be NULL or point to a free area in the heap
			if (heapEntry == NULL || count > heapEntry->size) {
				// Search through all the entries in the heap and find the first fit
				heapEntry = (HeapEntry*)((uint64_t)heap + sizeof(HeapHeader));
				freeEntryFound = false;
				while (heapEntry) {
					if (heapEntry->signature == HEAP_ENTRY_FREE && count <= heapEntry->size) {
						freeEntryFound = true;
						break;
					}
					heapEntry = nextHeapEntry(heap, heapEntry);
				}
			}
			if (freeEntryFound) {
				uint32_t originalSize = heapEntry->size;
				heapEntry->signature = HEAP_ENTRY_USED;
				heapEntry->size = count;
				heap->remaining -= count;
				++heap->entryCount;
				void *mallocedValue = (void*)((uint64_t)heapEntry + sizeof(HeapEntry));
				heap->entryTable[heap->entryCount - 1] = mallocedValue;
				HeapEntry *newEntry = nextHeapEntry(heap, heapEntry);
				if (newEntry == NULL) {
					// mallocedValue is the last entry
					heap->remaining = 0;
					heap->latestEntrySearched = NULL;
				} else if (newEntry->signature != HEAP_ENTRY_USED) {
					newEntry->signature = HEAP_ENTRY_FREE;
					newEntry->size = originalSize - count - sizeof(HeapEntry);
					heap->latestEntrySearched = newEntry;
				} else {
					heap->latestEntrySearched = NULL;
				}
				return mallocedValue;
			}
		}
		heap = heap->next;
	}

	// Traversed all the heap regions but couldn't find a free region big enough
	// TODO: Create a new heap region and use that
	return NULL;
}

void kernelFree(void *address) {
	uint64_t addr = (uint64_t) address;
	HeapHeader *heap = heapRegionsList;
	while (heap) {
		// Ensure the address is within the heap's bounds
		if (
			(addr >= (uint64_t)heap + sizeof(HeapHeader) + sizeof(HeapEntry)) &&
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
				heap->latestEntrySearched = (HeapEntry*)(addr - sizeof(HeapEntry));
				heap->latestEntrySearched->signature = HEAP_ENTRY_FREE;
				heap->remaining += heap->latestEntrySearched->size;
				// TODO: defrag the heap
				return;
			}
		}
		heap = heap->next;
	}
	terminalPrintString(invalidFreeStr, strlen(invalidFreeStr));
	terminalPrintHex(&address, sizeof(address));
	terminalPrintChar('\n');
	hangSystem(true);
}

static HeapEntry* nextHeapEntry(HeapHeader *heap, HeapEntry *heapEntry) {
	uint64_t heapEnd = (uint64_t)heap + heap->size;
	uint64_t newHeapEntry = (uint64_t)heapEntry + sizeof(HeapEntry) + heapEntry->size;
	if (newHeapEntry >= heapEnd || heapEnd - newHeapEntry - sizeof(HeapEntry) < HEAP_MIN_BLOCK_SIZE) {
		return NULL;
	}
	return (HeapEntry*)newHeapEntry;
}
