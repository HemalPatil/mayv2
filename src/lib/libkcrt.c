#include<stdint.h>
#include<stdbool.h>
#include<stddef.h>

size_t strlen(const char* str)
{
	size_t length = 0;
	while(*str++)
	{
		length++;
	}
	return length;
}

int64_t strcmp(const char* str1, const char* str2)
{
	while(*str1 && (*str1 == *str2))
	{
		str1++;
		str2++;
	}
	return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}