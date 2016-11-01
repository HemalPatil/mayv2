#include "kernel.h"

void OutputByte(uint16_t port, uint8_t data)
{
	asm("mov %sil, %al\n"	// register sil has the data to be output, di has the port number passed as arguments
		"mov %di, %dx\n"	// move these values to al and dx respectively
		"out %al, (%dx)");	// output the data in al at port number in dx
}