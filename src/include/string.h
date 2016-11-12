#pragma once

size_t strlen(const char* const str)
{
	size_t length = 0;
	while(str[length] != 0)
	{
		length++;
	}
	return length;
}