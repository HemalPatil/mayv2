#include "kernel.h"

bool SetupHardwareInterrupts()
{
	// TODO : add ioapic and hardware irq redirection
	if(!APICExists())
	{
		return false;
	}

	return true;
}

bool APICExists()
{
	asm("mov $0x1, %eax\n"
		"cpuid\n"
		"and $0x200, %edx\n"
		"shr $0x9, %edx\n"
		"xor %rax, %rax\n"
		"mov %edx, %eax");
}