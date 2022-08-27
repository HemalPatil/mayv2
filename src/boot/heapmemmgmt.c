#include <heapmemmgmt.h>

void* heapBase = 0;
uint64_t heapSize = 0x400000; // Start with 4MiB heap