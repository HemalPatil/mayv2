#pragma once
#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>

void KernelMain();
void TerminalClearScreen();
void TerminalSetCursor(size_t x, size_t y);