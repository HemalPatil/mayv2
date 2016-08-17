#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>

extern void test1(uint64_t, uint64_t);

void KernelMain()
{
	test1(0xdeadc0dedeadbeef, 0xcafebabeff123456);
}