#include "kernel.h"

const char* RSDPSignature = "RSD PTR ";

struct RSDPDescriptor2
{
	char signature[8];
	uint8_t checksum;
	char OEMID[6];
	uint8_t revision;
	uint32_t RSDTAddress;
	uint32_t length;
	uint64_t XSDTAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));

typedef struct RSDPDescriptor2 RSDPDescriptor2;

RSDPDescriptor2* GetRSDPPointer()
{
	asm("movabs $0x2052545020445352, %rax\n"
		"mov $0x400, %ecx\n"
		"mov $0x9fc00, %edx\n"
		"ebdaloop:\n"
		"mov (%rdx), %rbx\n"
		"cmp %rbx, %rax\n"
		"je found\n"
		"inc %rdx\n"
		"dec %ecx\n"
		"test %ecx, %ecx\n"
		"jz ebdaloop\n"
		"mov $0x20000, %ecx\n"
		"mov $0xe0000, %edx\n"
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

	return true;
}