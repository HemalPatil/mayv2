#pragma once

#include <stddef.h>

extern "C" size_t availableInterrupt;

extern "C" void enableInterrupts();
extern "C" void installIdt64Entry(size_t interruptNumber, void (*handler)());
extern "C" void setupIdt64();
