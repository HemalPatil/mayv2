#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "acpi.h"
#include "infotable.h"

#define PhyMemEntry_Free 0
#define PhyMemEntry_Used 1

struct PhyMemBitmapIndex {
	uint32_t byteIndex;
	uint32_t bitIndex;
};
typedef struct PhyMemBitmapIndex PhyMemBitmapIndex;

// System information table (see docs for more info)
extern InfoTable *infoTable;

// kernel.c
// do not expose the KernelMain function to other files
extern void kernelPanic();

// acpi.c
extern RSDPDescriptor2 *rsdp;
extern bool parseAcpi3();

// terminal.c
extern const char* const hexPalette;
extern bool isTerminalMode();
extern void terminalClearLine(size_t lineNumber);
extern void terminalClearScreen();
extern void terminalSetCursorPosition(size_t x, size_t y);
extern void terminalPrintString(const char* const str, const size_t length);
extern void terminalPrintChar(char);
extern void terminalPrintHex(void* value, size_t size);
extern void terminalPrintSpaces4();
extern void terminalScroll(size_t lineCount);

// interrupts.c
extern bool setupHardwareInterrupts();
extern bool apicExists();

// phymemmgmt.c
extern const size_t phyPageSize;
extern size_t phyPagesCount;
extern uint8_t* phyMemBitmap;
extern uint64_t phyMemBitmapSize;
extern uint64_t phyMemUsableSize;
extern uint64_t phyMemTotalSize;
extern PhyMemBitmapIndex getPhysicalPageBitmapIndex(void* address);
extern void initMmap();
extern void initPhysicalMemorySize();
extern void initUsablePhysicalMemorySize();
extern bool initializePhysicalMemory();
extern bool isPhysicalPageAvailable(void* address, size_t numberOfPages);
extern void listMmapEntries();
extern void listUsedPhysicalPages();
extern void markPhysicalPagesAsUsed(void* address, size_t numberOfPages);

// virtualmemmgmt.c
extern const size_t virPageSize;
extern bool initializeVirtualMemory();

// float.c
extern bool fpuExists();
//extern bool InitializeFPU();

// Assembly level functions
// apic.asm
extern void setupApic();

// kernellib.asm
extern uint8_t getLinearAddressLimit();
extern uint8_t getPhysicalAddressLimit();
extern void hangSystem();
extern void outputByte(uint16_t port, uint8_t byte);

// idt64.asm
extern void setupIdt64();
extern void enableInterrupts();

// tss64.asm
extern void setupTss64();

// Linker script symbols
extern const uint64_t GDT_START;
extern const uint64_t GDT_END;
extern const uint64_t IDT_START;
extern const uint64_t IDT_END;
extern const uint64_t TSS_START;
extern const uint64_t TSS_END;