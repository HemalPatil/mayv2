#include "kernel.h"

bool setupHardwareInterrupts() {
	// TODO : add ioapic and hardware irq redirection
	if(!apicExists()) {
		return false;
	}
	return true;
}

bool apicExists() {
	// TODO: convert to NASM code in kernellib.asm
	asm("mov $0x1, %eax\n"
		"cpuid\n"
		"and $0x200, %edx\n"
		"shr $0x9, %edx\n"
		"xor %rax, %rax\n"
		"mov %edx, %eax");
}