#pragma once

#include <acpi.h>
#include <infotable.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define K64_HIGHERHALF_ORIGIN 0xffffffff80000000
#define L32K64_SCRATCH_BASE 0x80000
#define L32K64_SCRATCH_LENGTH 0x10000

// kernel.c
// do not expose the KernelMain function to other files
extern InfoTable *infoTable;

extern void kernelPanic();


// kernellib.asm
extern uint8_t getLinearAddressLimit();
extern uint8_t getPhysicalAddressLimit();
extern void hangSystem();
