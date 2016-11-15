#include "kernel.h"

bool SetupHardwareInterrupts()
{
	// TODO : add ioapic and hardware irq redirection
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

bool SetupInterrupts()
{
	PopulateIDTWithOffsets();

	if(!APICExists())
	{
		return false;
	}

	return SetupHardwareInterrupts();
}