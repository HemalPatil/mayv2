#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

// Represent the strings "FREE" and "USED"
#define HEAP_ENTRY_FREE 0x45455246
#define HEAP_ENTRY_USED 0x44455355

HeapHeader *heapRegionsList = INVALID_ADDRESS;
HeapHeader *latestHeapSearched = INVALID_ADDRESS;

// Start with 4MiB heap always,
// must always be a multiple of 2MiB due to current limitations in physical memory manager
uint64_t initialHeapSize = 0x400000;

static const char* const initHeapStr = "Initializing dynamic memory management...\n";
static const char* const initHeapCompleteStr = "Dynamic memory management initialized\n\n";
static const char* const creatingHeapHeaderStr = "Creating heap headers...";
static const char* const movingVirMemLists = "Moving virtual address space lists to heap memory...";
static const char* const listHeapStr = "List of all kernel heap regions\n";
static const char* const heapHeaderStr = "Heap start           Size                 Count Remaining\n";

bool initializeDynamicMemory() {
	// TODO: complete dynamic memory initialization
	terminalPrintString(initHeapStr, strlen(initHeapStr));

	// Split the 4MiB heap reserved by virtual memory manager during its initialization
	// into 2 heaps of size 2MiB each
	terminalPrintSpaces4();
	terminalPrintString(creatingHeapHeaderStr, strlen(creatingHeapHeaderStr));
	HeapHeader *nextHeap = (HeapHeader*)((uint64_t)heapRegionsList + MIB_2);
	heapRegionsList->previous = nextHeap->next = NULL;
	heapRegionsList->next = nextHeap;
	heapRegionsList->entryCount = nextHeap->entryCount = 0;
	heapRegionsList->size = nextHeap->size = MIB_2;
	heapRegionsList->remaining = nextHeap->remaining = heapRegionsList->size - sizeof(HeapHeader) - 2 * sizeof(uint32_t);
	nextHeap->previous = latestHeapSearched = heapRegionsList;
	uint32_t *entrySignature = (uint32_t*)((uint64_t)heapRegionsList + sizeof(HeapHeader));
	*(entrySignature++) = HEAP_ENTRY_FREE;
	*(entrySignature++) = heapRegionsList->remaining;
	heapRegionsList->latestEntrySearched = entrySignature;
	entrySignature = (uint32_t*)((uint64_t)nextHeap + sizeof(HeapHeader));
	*(entrySignature++) = HEAP_ENTRY_FREE;
	*(entrySignature++) = nextHeap->remaining;
	heapRegionsList->latestEntrySearched = entrySignature;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// TODO: Move the virtual address space lists at the end of 2nd heap
	// to the 1st heap to make them dynamic
	terminalPrintSpaces4();
	terminalPrintString(movingVirMemLists, strlen(movingVirMemLists));
	// kernelFree(NULL);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	listAllHeapRegions();

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
