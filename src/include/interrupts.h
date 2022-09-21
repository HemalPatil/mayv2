#pragma once

#include <stdbool.h>

#define IRQ_KEYBOARD 1
#define IRQ_AHCI 9

extern void endInterrupt();
extern bool initializeInterrupts();