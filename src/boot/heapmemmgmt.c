#include <heapmemmgmt.h>
#include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

void* heapBase = 0;
uint64_t heapSize = 0x400000; // Start with 4MiB heap, must always be a multiple of 2MiB

const char* const initHeapStr = "Initializing dynamic memory management...\n";
const char* const initHeapCompleteStr = "Dynamic memory management initialized\n\n";
const char* const heapBaseStr = "HeapBase ";

bool initializeDynamicMemory() {
	terminalPrintString(initHeapStr, strlen(initHeapStr));

	// TODO: complete dynamic memory initialization
	terminalPrintSpaces4();
	terminalPrintString(heapBaseStr, strlen(heapBaseStr));
	terminalPrintHex(&heapBase, sizeof(heapBase));
	terminalPrintChar('\n');

	terminalPrintString(initHeapCompleteStr, strlen(initHeapCompleteStr));
	return true;
}