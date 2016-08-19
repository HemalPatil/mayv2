#include"kernel.h"

void KernelMain()
{
	char* const vidmem = (char*)0xb8000;
	vidmem[398] = 'F';
	vidmem[3998] = 'F';
	TerminalClearScreen();
	TerminalSetCursor(0, 0);
}