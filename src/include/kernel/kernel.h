#pragma once

#include <acpi.h>
#include <infotable.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GIB_1 (uint64_t)1024 * 1024 * 1024
#define KIB_4 4 * 1024
#define MIB_2 2 * 1024 * 1024

#define INVALID_ADDRESS ((void*) 0x8000000000000000)
#define KERNEL_LOWERHALF_ORIGIN 0x80000000
#define KERNEL_HIGHERHALF_ORIGIN 0xffffffff80000000
#define L32K64_SCRATCH_BASE 0x80000
#define L32K64_SCRATCH_LENGTH 0x10000

// kernel.cpp
// do not expose the kernelMain to other files
extern InfoTable *infoTable;

extern "C" [[noreturn]] void kernelPanic();


// kernelasm.asm
extern "C" void flushTLB(void *newPml4Root);
extern "C" [[noreturn]] void hangSystem(bool disableInterrupts);
