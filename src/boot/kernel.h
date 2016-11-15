#pragma once
#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>
// custom c standard library includes


#define ACPI3_MemType_Usable 1
#define ACPI3_MemType_Reserved 2
#define ACPI3_MemType_ACPIReclaimable 3
#define ACPI3_MemType_ACPINVS 4
#define ACPI3_MemType_Bad 5
#define ACPI3_MemType_Hole 10

// ACPI 3.0 entry format (we have used extended entries of 24 bytes)
struct ACPI3Entry
{
	uint64_t BaseAddress;
	uint64_t Length;
	uint32_t RegionType;
	uint32_t ExtendedAttributes;
} __attribute__((packed));

// System information table (see docs for more info)
uint16_t *InfoTable;

// kernel.c
void KernelMain(uint16_t *InfoTableAddress);
void KernelPanic();

// acpi.c
bool InitializeACPI3();

// terminal.c
bool IsTerminalMode();
void TerminalClearScreen();
void TerminalSetCursorPosition(size_t x, size_t y);
void TerminalPrintString(const char* const str, const size_t length);
void TerminalPutChar(char);

// interrupts.c
bool SetupInterrupts();
bool APICExists();

// phymemmgmt.c
struct ACPI3Entry* GetMMAPBase();
size_t GetNumberOfMMAPEntries();
uint64_t GetPhysicalMemorySize();
uint64_t GetUsablePhysicalMemorySize();
uint64_t GetKernelBasePhysicalMemory();
bool InitializePhysicalMemory();

// virtualmemmgmt.c
uint64_t GetKernelSize();
bool InitializeVirtualMemory();

// float.c
bool FPUExists();
//bool InitializeFPU();

// io.c
void OutputByte(uint16_t port, uint8_t byte);

// Assembly level functions
// kernellib.asm
extern void HangSystem();

// idt64.asm
extern void PopulateIDTWithOffsets();

// Linker script symbols
extern const uint64_t __GDT_START;
extern const uint64_t __GDT_END;
extern const uint64_t __IDT_START;
extern const uint64_t __IDT_END;
extern const uint64_t __TSS_START;
extern const uint64_t __TSS_END;