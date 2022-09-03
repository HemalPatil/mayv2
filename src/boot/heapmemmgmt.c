#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static HeapEntry* nextHeapEntry(HeapHeader *heap, HeapEntry *heapEntry);

HeapHeader *heapRegionsList = INVALID_ADDRESS;
HeapHeader *latestHeapSearched = INVALID_ADDRESS;

static const char* const initHeapStr = "Initializing dynamic memory management...\n";
static const char* const initHeapCompleteStr = "Dynamic memory management initialized\n\n";
static const char* const creatingHeapHeaderStr = "Creating heap header...";
static const char* const movingVirMemLists = "Moving virtual address space lists to heap memory...";
static const char* const listHeapStr = "List of all kernel heap regions\n";
static const char* const heapHeaderStr = "Heap start           Size                 Count Remaining\n";
static const char* const invalidFreeStr = "\nInvalid kernel free operation ";

bool initializeDynamicMemory() {
	// TODO: complete dynamic memory initialization
	terminalPrintString(initHeapStr, strlen(initHeapStr));

	// Initalize the heap region reserved by virtual memory manager during its initialization
	terminalPrintSpaces4();
	terminalPrintString(creatingHeapHeaderStr, strlen(creatingHeapHeaderStr));
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

	// TODO: Move the virtual address space lists at the end of heap to the heap to make them dynamic
	// and unmap the page containing the lists
	terminalPrintSpaces4();
	terminalPrintString(movingVirMemLists, strlen(movingVirMemLists));
	void *ghostPage = kernelAddressSpaceList;
	VirtualMemNode *lists[2] = { kernelAddressSpaceList, normalAddressSpaceList };
	VirtualMemNode *newLists[2] = { NULL, NULL };
	for (size_t i = 0; i < 2; ++i) {
		VirtualMemNode *current = kernelMalloc(sizeof(VirtualMemNode));
		memcpy(current, lists[i], sizeof(VirtualMemNode));
		newLists[i] = current;
		VirtualMemNode *newPrevious = current;
		VirtualMemNode *oldCurrent = lists[i]->next;
		while (oldCurrent) {
			current = kernelMalloc(sizeof(VirtualMemNode));
			memcpy(current, oldCurrent, sizeof(VirtualMemNode));
			newPrevious->next = current;
			current->previous = newPrevious;
			newPrevious = current;
			oldCurrent = oldCurrent->next;
		}
	}
	kernelAddressSpaceList = newLists[0];
	normalAddressSpaceList = newLists[1];
	// if (!unmapVirtualPages(ghostPage, 1, true)) {
	// 	terminalPrintString(failedStr, strlen(failedStr));
	// 	terminalPrintChar('\n');
	// 	return false;
	// }
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	listKernelAddressSpace();
	listNormalAddressSpace();

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
	// All heap regions are currently of size 2MiB so serving more than that is not possible
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
				heap->entryTable[heap->entryCount] = mallocedValue;
				HeapEntry *newEntry = nextHeapEntry(heap, heapEntry);
				if (newEntry == NULL) {
					// mallocedValue is the last entry
					heap->remaining = 0;
					heap->latestEntrySearched = NULL;
				} else if (newEntry->signature != HEAP_ENTRY_USED) {
					newEntry->signature = HEAP_ENTRY_FREE;
					newEntry->size = originalSize - count - sizeof(HeapEntry);
					heap->latestEntrySearched = newEntry;
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

// void someRandomName(void *address) {
// 	uint64_t addr = (uint64_t) address;
// 	HeapHeader *header = heapRegionsList;
// 	while (header) {
// 		// Ensure the address is within the heap's bounds
// 		uint64_t heapStart = (uint64_t) header;
// 		uint64_t heapFreeEnd = heapStart + header->size - header->entryCount * sizeof(void*);
// 		if (
// 			(addr > heapStart + sizeof(HeapHeader) + 2 * sizeof(uint32_t)) &&
// 			(addr < heapFreeEnd)
// 		) {
// 			// Check if this address is allocated in the current heap
// 			bool entryFound = false;
// 			uint64_t *entry = (uint64_t*)heapFreeEnd;
// 			for (size_t i = 0; i < header->entryCount; ++i, ++entry) {
// 				if (*entry == addr) {
// 					entryFound = true;
// 					break;
// 				}
// 			}
// 			if (entryFound) {
// 				--header->entryCount;
// 				*entry = *((uint64_t*)heapFreeEnd);
// 			}
// 		}
// 		header = header->next;
// 	}
// 	terminalPrintString(invalidFreeStr, strlen(invalidFreeStr));
// 	terminalPrintChar('\n');
// }

static HeapEntry* nextHeapEntry(HeapHeader *heap, HeapEntry *heapEntry) {
	uint64_t heapEnd = (uint64_t)heap + heap->size;
	uint64_t newHeapEntry = (uint64_t)heapEntry + sizeof(HeapEntry) + heapEntry->size;
	if (newHeapEntry >= heapEnd || heapEnd - newHeapEntry - sizeof(HeapEntry) < HEAP_MIN_BLOCK_SIZE) {
		return NULL;
	}
	return (HeapEntry*)newHeapEntry;
}