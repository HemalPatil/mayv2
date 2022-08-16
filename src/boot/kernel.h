#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ACPI3_MemType_Usable 1
#define ACPI3_MemType_Reserved 2
#define ACPI3_MemType_ACPIReclaimable 3
#define ACPI3_MemType_ACPINVS 4
#define ACPI3_MemType_Bad 5
#define ACPI3_MemType_Hole 10

#define RSDP_Revision_1 0
#define RSDP_Revision_2_and_above 1

// ACPI 3.0 entry format (we have used extended entries of 24 bytes)
struct ACPI3Entry {
	uint64_t baseAddress;
	uint64_t length;
	uint32_t regionType;
	uint32_t extendedAttributes;
} __attribute__((packed));
typedef struct ACPI3Entry ACPI3Entry;

struct ACPISDTHeader {
	// Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2.8
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemId[6];
	char oemTableID[8];
	uint32_t oemRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;
} __attribute__((packed));
typedef struct ACPISDTHeader ACPISDTHeader;

struct RSDPDescriptor2 {
	char signature[8];
	uint8_t checksum;
	char oemId[6];
	uint8_t revision;
	uint32_t rsdtAddress;
	uint32_t length;
	ACPISDTHeader *xsdtAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));
typedef struct RSDPDescriptor2 RSDPDescriptor2;

struct InfoTable {
	uint16_t bookDiskNumber;
	uint16_t numberOfMmapEntries;
	uint16_t mmapEntriesOffset;
	uint16_t mmapEntriesSegment;
	uint16_t vesaInfoOffset;
	uint16_t vesaInfoSegment;
	char gdt32Descriptor[6];
	uint16_t maxPhysicalAddress;
	uint16_t maxLinearAddress;
	uint64_t kernel64VirtualMemSize;
	void* kernel64Base;
} __attribute__((packed));
typedef struct InfoTable InfoTable;

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