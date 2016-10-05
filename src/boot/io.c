#include "kernel.h"

void OutputByte(uint16_t port, uint8_t data)
{
	asm("mov %sil, %al\n"
		"mov %di, %dx\n"
		"out %al, (%dx)");	// register sil has the data to be output, di has the port number passed as arguments
}