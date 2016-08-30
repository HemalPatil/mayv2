#include"kernel.h"

void test(char x)
{
	char* const vidmem = (char*)0xb8000;
	vidmem[0] = x;
	vidmem[1] = 0x0f;
}