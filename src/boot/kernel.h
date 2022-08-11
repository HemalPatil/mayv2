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
struct ACPI3Entry
{
	uint64_t BaseAddress;
	uint64_t Length;
	uint32_t RegionType;
	uint32_t ExtendedAttributes;
} __attribute__((packed));

struct RSDPDescriptor2
{
	char signature[8];
	uint8_t checksum;
	char OEMID[6];
	uint8_t revision;
	uint32_t RSDTAddress;
	uint32_t length;
	struct ACPISDTHeader *XSDTAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));
typedef struct RSDPDescriptor2 RSDPDescriptor2;

struct ACPISDTHeader
{
	// Read https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf Section 5.2.8
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} __attribute__((packed));
typedef struct ACPISDTHeader ACPISDTHeader;

// System information table (see docs for more info)
extern uint16_t *InfoTable;

// kernel.c
// do not expose the KernelMain function to other files
extern void KernelPanic();

// acpi.c
extern struct RSDPDescriptor2 *rsdp;
extern bool ParseACPI3();

// terminal.c
extern const char* const HexPalette;
extern bool IsTerminalMode();
extern void TerminalClearScreen();
extern void TerminalSetCursorPosition(size_t x, size_t y);
extern void TerminalPrintString(const char* const str, const size_t length);
extern void TerminalPutChar(char);
extern void TerminalPrintHex(void* value, size_t size);

// interrupts.c
extern bool SetupHardwareInterrupts();
extern bool APICExists();

// phymemmgmt.c
extern struct ACPI3Entry* GetMMAPBase();
extern size_t GetNumberOfMMAPEntries();
extern uint64_t GetPhysicalMemorySize();
extern uint64_t GetUsablePhysicalMemorySize();
extern uint64_t GetKernelBasePhysicalMemory();
extern bool InitializePhysicalMemory();
//extern void AllocatePhysicalMemoryContiguous();
//extern void AllocatePhysicalMemory();
extern void MarkPhysicalPagesAsUsed(uint64_t address, size_t NumberOfPages);
extern bool IsPhysicalPageAvailable(uint64_t address, size_t NumberOfPages);

// virtualmemmgmt.c
extern uint64_t GetKernelSize();
extern bool InitializeVirtualMemory();

// float.c
extern bool FPUExists();
//extern bool InitializeFPU();

// Assembly level functions
// apic.asm
extern void SetupAPIC();

// kernellib.asm
extern uint8_t GetLinearAddressLimit();
extern uint8_t GetPhysicalAddressLimit();
extern void HangSystem();
extern void OutputByte(uint16_t port, uint8_t byte);
extern RSDPDescriptor2* SearchRSDP();
extern void TerminalClearScreenASM();

// idt64.asm
extern void SetupIDT64();
extern void EnableInterrupts();

// tss64.asm
extern void SetupTSS64();

// Linker script symbols
extern const uint64_t __GDT_START;
extern const uint64_t __GDT_END;
extern const uint64_t __IDT_START;
extern const uint64_t __IDT_END;
extern const uint64_t __TSS_START;
extern const uint64_t __TSS_END;