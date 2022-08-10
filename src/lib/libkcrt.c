#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

size_t strlen(const char* str)
{
	size_t length = 0;
	while(*str++)
	{
		length++;
	}
	return length;
}

int64_t strcmp(const char* str1, const char* str2) {
	while (*str1 && (*str1 == *str2)) {
		++str1;
		++str2;
	}
	return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

void memset(void* const address /*rdi*/, const uint8_t data /*rsi*/, const size_t length /*rdx*/) {
	// TODO: convert to NASM syntax
	asm("test %rdx,%rdx\n"	// check if length is 0
	"je memsetEnd\n"
	"mov %rdx,%rax\n"	// get length in rax
	"xor %rdx,%rdx\n"	// 0 rdx for division
	"mov $0x8,%ecx\n"	// get copy of 8 in rcx and r10
	"mov %rcx,%r10\n"
	"div %rcx\n"	// get qword moves in rax
	"mov %rdx,%r8\n"	// copy remainder in r8
	"test %rax,%rax\n"	// check if number of qword moves are 0
	"je memsetQWORDskip\n"
	"mov %rax,%r9\n"	// keep a copy of qword moves in r9
	"mov %r10,%rcx\n"
	"memsetRAXloop:\n"	// set rax to sil 8 times
	"shl $0x8,%rax\n"
	"mov %sil,%al\n"
	"dec %rcx\n"
	"test %rcx,%rcx\n"
	"je memsetRAXloop\n"
	"mov %r9,%rcx\n"	// get qword moves back in rcx
	"rep stos %rax,%es:(%rdi)\n"	// do 8 byte stores
	"memsetQWORDskip:\n"
	"mov %r8,%rcx\n"	// get single byte moves in rcx
	"mov %sil,%al\n"
	"rep stos %al,%es:(%rdi)\n"	// do byte stores
	"memsetEnd:\n");
}