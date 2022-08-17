#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "acpi.h"
#include "infotable.h"

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
extern void terminalScroll(size_t lineCount);

// interrupts.c
extern bool setupHardwareInterrupts();
extern bool apicExists();

// phymemmgmt.c
extern ACPI3Entry* getMmapBase();
extern size_t getNumberOfMmapEntries();
extern uint64_t getPhysicalMemorySize();
extern uint64_t getUsablePhysicalMemorySize();
extern uint64_t getKernelBasePhysicalMemory();
extern bool initializePhysicalMemory();
//extern void AllocatePhysicalMemoryContiguous();
//extern void AllocatePhysicalMemory();
extern void markPhysicalPagesAsUsed(uint64_t address, size_t numberOfPages);
extern bool isPhysicalPageAvailable(uint64_t address, size_t numberOfPages);

// virtualmemmgmt.c
extern uint64_t getKernelSize();
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