#pragma once

#include <stdbool.h>
#include <stddef.h>

#define IRQ_KEYBOARD 1
#define IRQ_AHCI 9

extern size_t availableInterrupt;

extern void acknowledgeLocalApicInterrupt();
extern bool initializeInterrupts();
