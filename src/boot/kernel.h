#pragma once
#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>
#include "acpi.h"

// System information table (see docs for more info)
uint16_t *InfoTable;

// kernel.c
// do not expose the KernelMain function to other files
// void KernelMain(uint16_t *InfoTableAddress);
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