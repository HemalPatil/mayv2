#include "kernel.h"

RSDPDescriptor2* GetRSDPPointer()
{
	asm("movabs $0x2052545020445352, %rax\n"
		"mov $0x400, %ecx\n"
		"movabs $0x9fc00, %rdx\n"
		"ebdaloop:\n"
		"mov (%rdx), %rbx\n"
		"cmp %rbx, %rax\n"
		"je found\n"
		"inc %rdx\n"
		"dec %ecx\n"
		"test %ecx, %ecx\n"
		"jz ebdaloop\n"
		"mov $0x20000, %ecx\n"
		"movabs $0xe0000, %rdx\n"
		"memloop:\n"
		"mov (%rdx), %rbx\n"
		"cmp %rbx, %rax\n"
		"je found\n"
		"inc %rdx\n"
		"dec %ecx\n"
		"test %ecx, %ecx\n"
		"jz memloop\n"
		"mov $0x0, %eax\n"
		"jmp GetRSDPPointerExit\n"
		"found:\n"
		"mov %rdx, %rax\n"
		"GetRSDPPointerExit:");
}

bool InitializeACPI3()
{
	RSDPDescriptor2 *rsdp = GetRSDPPointer();
	if(!rsdp)
	{
		return false;
	}

	if(rsdp->revision == RSDP_Revision_1)
	{
		return false;
	}

	// Verify the checksum
	uint8_t checksum = 0;
	for(size_t i=0; i<sizeof(RSDPDescriptor2); i++)
	{
		checksum += ((uint8_t*)rsdp)[i];
	}
	if(checksum != 0)
	{
		return false;
	}

	// will generate page fault because not memory mapped
	uint8_t x = *((uint8_t*)rsdp->XSDTAddress);

	return true;
}