#pragma once

#include <stdbool.h>

#define IRQ_KEYBOARD 1

extern void endInterrupt();
extern bool initializeInterrupts();