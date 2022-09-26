#pragma once

extern void enableInterrupts();
extern void installIdt64Entry(size_t interruptNumber, void* handler);
extern void setupIdt64();