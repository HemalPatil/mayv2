#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "acpi.h"
#include "elf64.h"
#include "infotable.h"
#include "pml4t.h"

#define PML4E_ROOT 0x110000

// Disk Address Packet for extended disk operations
struct DAP {
	char dapSize;
	char alwayszero;
	uint16_t numberOfSectors;
	uint16_t offset;
	uint16_t segment;
	uint64_t firstSector;
} __attribute__((packed));
typedef struct DAP DAP;

char *const vidMem = (char *)0xb8000;
const size_t vgaWidth = 80;
const size_t vgaHeight = 25;
size_t cursorX = 0;
size_t cursorY = 0;
const char endl[] = "\n";
const uint8_t DEFAULT_TERMINAL_COLOR = 0x0f;
InfoTable *infoTable;

void clearScreen();
extern void printHex(const void *const, const size_t);
extern void swap(void *const a, void *const b, const size_t);
extern void memset(void *const dest, const uint8_t, const size_t);
extern void memcpy(const void *const src, void const *dest, const size_t count);
extern void setup16BitSegments(const InfoTable* const table, const void *const loadModule, const DAP* const dap, const uint16_t);
extern void loadKernel64ElfSectors();
extern void jumpToKernel64(const PML4E* const pml4t, const InfoTable* const table);
extern uint8_t getLinearAddressLimit();
extern uint8_t getPhysicalAddressLimit();

void terminalPrintChar(char c, uint8_t color) {
	if (cursorX >= vgaWidth || cursorY >= vgaHeight) {
		return;
	}
	if (c == '\n') {
		goto uglySkip;
	}
	const size_t index = 2 * (cursorY * vgaWidth + cursorX);
	vidMem[index] = c;
	vidMem[index + 1] = color;
	if (++cursorX == vgaWidth) {
		uglySkip:
			cursorX = 0;
			if (++cursorY == vgaHeight) {
				cursorY = 0;
				clearScreen();
			}
	}
	return;
}

void clearScreen() {
	const size_t buffersize = 2 * vgaWidth * vgaHeight;
	for (size_t i = 0; i < buffersize; i = i + 2) {
		vidMem[i] = ' ';
		vidMem[i + 1] = DEFAULT_TERMINAL_COLOR;
	}
	cursorX = cursorY = 0;
	return;
}

size_t strlen(const char *const str) {
	size_t length = 0;
	while (str[length] != 0) {
		++length;
	}
	return length;
}

void printString(const char *const str) {
	size_t length = strlen(str);
	for (size_t i = 0; i < length; ++i) {
		terminalPrintChar(str[i], DEFAULT_TERMINAL_COLOR);
	}
}

ACPI3Entry* getMmapBase() {
	return (ACPI3Entry*)((uint32_t)(infoTable->mmapEntriesSegment << 4) + infoTable->mmapEntriesOffset);
}

void sortMmapEntries() {
	// Sort the MMAP entries in ascending order of their base addresses
	// Number of MMAP entries is in the range of 5-15 typically so a simple bubble sort is used
	ACPI3Entry *mmap = getMmapBase();
	for (size_t i = 0; i < infoTable->mmapEntryCount - 1; ++i) {
		size_t iMin = i;
		for (size_t j = i + 1; j < infoTable->mmapEntryCount; ++j) {
			uint64_t basej = *((uint64_t *)(mmap + j));
			uint64_t baseMin = *((uint64_t *)(mmap + iMin));
			if (basej < baseMin) {
				iMin = j;
			}
		}
		if (iMin != i) {
			swap(mmap + i, mmap + iMin, 24);
		}
	}
}

void processMmapEntries() {
	// This function checks for any overlaps in the memory regions provided by BIOS
	// Overlaps occur in very rare cases. Nevertheless they must be handled.
	// If an overlap is found, two cases occur
	// 1) Both regions are of same type
	//		- Change the length of the one entry to match the base of next entry
	// 2) Regions are of different types and one of them is of type ACPI3_MemType_Usable
	//		- Change the length of the first entry if it is usable
	//		- Change the base of the second entry if it is usable
	// 3) Overlapping of two regions of different types and none of them of type ACPI3_MemType_Usable
	//    is not handled right now.

	ACPI3Entry *mmap = getMmapBase();
	size_t actualEntries = infoTable->mmapEntryCount;
	sortMmapEntries();
	for (size_t i = 0; i < infoTable->mmapEntryCount - 1; ++i) {
		ACPI3Entry *mmapEntry1 = mmap + i;
		ACPI3Entry *mmapEntry2 = mmap + i + 1;
		if ((mmapEntry1->baseAddress + mmapEntry1->length) > mmapEntry2->baseAddress) {
			if (mmapEntry1->regionType == mmapEntry2->regionType) {
				mmapEntry1->length = mmapEntry2->baseAddress - mmapEntry1->baseAddress;
			} else {
				if (mmapEntry1->regionType == ACPI3_MemType_Usable) {
					mmapEntry1->length = mmapEntry2->baseAddress - mmapEntry1->baseAddress;
				} else if (mmapEntry2->regionType == ACPI3_MemType_Usable) {
					mmapEntry2->baseAddress = mmapEntry1->baseAddress + mmapEntry1->length;
				}
			}
		} else if ((mmapEntry1->baseAddress + mmapEntry1->length) < mmapEntry2->baseAddress) {
			ACPI3Entry *newEntry = mmap + actualEntries;
			newEntry->baseAddress = mmapEntry1->baseAddress + mmapEntry1->length;
			newEntry->length = mmapEntry2->baseAddress - newEntry->baseAddress;
			newEntry->regionType = ACPI3_MemType_Hole;
			newEntry->extendedAttributes = 1;
			++actualEntries;
		}
	}
	infoTable->mmapEntryCount = actualEntries;
	sortMmapEntries();
}

void identityMapFirst16MiB() {
	// Identity map first 16 MiB of the physical memory
	// pagePtr right now points to PML4T at 0x110000;
	uint64_t *pagePtr = (uint64_t *)PML4E_ROOT;
	infoTable->pml4eRoot = pagePtr;

	// Clear 22 4KiB pages
	memset(pagePtr, 0, 0xf0 * 4096);

	// PML4T[0] points to PDPT at 0x111000, i.e. the first 512GiB are handled by PDPT at 0x111000
	pagePtr[0] = (uint64_t)(PML4E_ROOT + 0x1003);

	// pagePtr right now points to PDPT at 0x111000 which handles first 512GiB
	pagePtr = (uint64_t *)(PML4E_ROOT + 0x1000);
	// PDPT[0] points to PD at 0x112000, i.e. the first 1GiB is handled by PD at 0x112000
	pagePtr[0] = (uint64_t)(PML4E_ROOT + 0x2003);

	// Each entry in PD points to PT. Each PT handles 2MiB.
	// Add 8 entries to PD and fill all entries in the PTs starting from 0x113000 to 0x11afff with page_frames 0x0 to 16MiB
	uint64_t *pdEntry = (uint64_t *)(PML4E_ROOT + 0x2000);;
	uint64_t *ptEntry = (uint64_t *)(PML4E_ROOT + 0x3000);;
	uint64_t pageFrame = (uint64_t)0x0003;
	size_t i = 0, j = 0;
	for (i = 0; i < 8; ++i) {
		pdEntry[i] = (uint64_t)((uint32_t)ptEntry + 3);
		for (j = 0; j < 512; ++j, ++ptEntry) {
			*(ptEntry) = pageFrame;
			pageFrame += 0x1000;
		}
	}
}

void allocatePagingEntry(PML4E *entry, uint32_t address) {
	entry->present = 1;
	entry->readWrite = 1;
	entry->pageAddress = address >> 12;
}

int loader32Main(InfoTable *infoTableAddress, DAP *const dapKernel64, const void *const loadModuleAddress)
{
	infoTable = infoTableAddress;
	clearScreen();
	processMmapEntries();

	// Get the max physical address and max linear address that can be handled by the CPU
	// These details are found by using CPUID.EAX=0x80000008 instruction and has to be done from assembly
	// Refer to Intel documentation Vol. 3 Section 4.1.4
	uint8_t maxPhyAddr = getPhysicalAddressLimit();
	printString("Max physical address = 0x");
	printHex(&maxPhyAddr, sizeof(maxPhyAddr));
	printString("\n");
	uint8_t maxLinAddr = getLinearAddressLimit();
	printString("Max linear address = 0x");
	printHex(&maxLinAddr, sizeof(maxLinAddr));
	printString("\n");
	infoTable->maxPhysicalAddress = infoTable->maxLinearAddress = 0;
	infoTable->maxPhysicalAddress = (uint16_t)maxPhyAddr;
	infoTable->maxLinearAddress = (uint16_t)maxLinAddr;

	uint16_t kernelNumberOfSectors = dapKernel64->numberOfSectors;
	uint32_t bytesOfKernelElf = (uint32_t)0x800 * (uint32_t)kernelNumberOfSectors;
	uint64_t kernelVirtualMemSize = infoTable->kernel64VirtualMemSize; // Get size of kernel in virtual memory

	// Check if enough space is available to load kernel process, kernel ELF, and PML4T
	// (i.e. length of region > (kernelVirtualMemSize + bytesOfKernelElf + 2MiB- baseAddress))
	// Load parsed kernel code from 2MiB physical memory (size : kernelVirtualMemSize)
	// Kernel ELF will be loaded at 2MiB + kernelVirtualMemSize + 4KiB physical memory from where it will be parsed
	bool enoughSpace = false;
	printString("Number of MMAP entries = 0x");
	printHex(&infoTable->mmapEntryCount, sizeof(infoTable->mmapEntryCount));
	printString("\n");
	ACPI3Entry *mmap = getMmapBase();
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (
			(mmap[i].baseAddress <= (uint64_t)0x100000) &&
			(mmap[i].length > ((uint64_t)0x200000 - mmap[i].baseAddress + 0x1000 + (uint64_t)bytesOfKernelElf + kernelVirtualMemSize))
		) {
			enoughSpace = true;
			break;
		}
	}
	if (!enoughSpace) {
		clearScreen();
		printString("\nFatal Error : System memory is fragmented too much.\nNot enough space to load kernel.\nCannot boot!");
		return 1;
	}
	printString("Loading kernel...\n");

	// Identity map the first 16 MiB of the physical memory
	// To see how the mapping is done refer to docs/mapping.txt
	identityMapFirst16MiB();

	// Change base address of the 16-bit segments in GDT32
	setup16BitSegments(infoTableAddress, loadModuleAddress, dapKernel64, infoTable->bookDiskNumber);

	// Enter the kernel physical memory base address in the info table
	uint32_t kernelBase = 0x200000;
	infoTable->kernel64Base = (uint64_t)kernelBase;

	// TODO: assumes memory from 0x80000 to 0x90000 is free
	// There is 64 KiB free in physical memory from 0x80000 to 0x90000. The sector in the OS ISO image is 2 KiB in size.
	// So the kernel ELF can be loaded in batches of 32 sectors. Leave a gap of 4 KiB between kernel process and kernel ELF
	uint32_t kernelElfBase = 0x200000 + 0x1000 + (uint32_t)kernelVirtualMemSize;
	printString("KernelVirtualMemSize = 0x");
	printHex(&kernelVirtualMemSize, sizeof(kernelVirtualMemSize));
	printString("\n");
	printString("KernelELFBase = 0x");
	printHex(&kernelElfBase, sizeof(kernelElfBase));
	printString("\n");
	dapKernel64->offset = 0x0;
	dapKernel64->segment = 0x8000;
	dapKernel64->numberOfSectors = 32;
	uint16_t iters = kernelNumberOfSectors / 32;
	memset((void *)0x80000, 0, 0x10000);
	for (uint16_t i = 0; i < iters; ++i) {
		loadKernel64ElfSectors();
		memcpy((void *)0x80000, (void *)(kernelElfBase + i * 0x10000), 0x10000);
		dapKernel64->firstSector += 32;
	}
	// Load remaining sectors
	dapKernel64->numberOfSectors = kernelNumberOfSectors % 32;
	loadKernel64ElfSectors();
	memcpy((void *)0x80000, (void *)(kernelElfBase + iters * 0x10000), dapKernel64->numberOfSectors * 0x800);

	printString("Kernel executable loaded.\n");

	// Parse the kernel executable
	ELF64Header *elfHeader = (ELF64Header*)kernelElfBase;
	if (elfHeader->endianness != ELF_LittleEndian || elfHeader->isX64 != ELF_Type_x64) {
		// Make sure kernel executable is 64-bit in little endian format
		printString("\nKernel executable corrupted (ELF header incorrect)! Cannot boot!");
		return 1;
	}
	if (elfHeader->headerEntrySize != sizeof(ELF64ProgramHeader)) {
		printString("\nKernel executable corrupted (header entry size mismatch)! Cannot boot!");
		return 1;
	}
	ELF64ProgramHeader *programHeader = (ELF64ProgramHeader *)(kernelElfBase + elfHeader->headerTablePosition);
	uint32_t memorySeekp = kernelBase;
	printString("KernelBase = 0x");
	printHex(&kernelBase, sizeof(kernelBase));
	printString("\n");
	printString("ProgramHeaderEntryCount = 0x");
	printHex(&elfHeader->headerEntryCount, sizeof(elfHeader->headerEntryCount));
	printString("\n");
	PML4E *pml4t = (PML4E *)PML4E_ROOT;
	uint32_t newPageStart = 0x11b000; // New pages that need to be made should start from this address and add 0x1000 to it.
	for (uint16_t i = 0; i < elfHeader->headerEntryCount; ++i) {
		uint32_t sizeInMemory = (uint32_t)programHeader[i].segmentSizeInMemory;
		printString("  SizeInMemory = 0x");
		printHex(&sizeInMemory, sizeof(sizeInMemory));
		printString("\n");

		memset((void *)memorySeekp, 0, sizeInMemory);
		memcpy((void *)(kernelElfBase + (uint32_t)programHeader[i].fileOffset), (void *)memorySeekp, (uint32_t)programHeader[i].segmentSizeInFile);

		// Kernel64 is linked at higher half addresses.
		// Right now it is last 2GiB of 64-bit address space, may change if linker script is changed
		// Map this section in the paging structure
		size_t numberOfPages = sizeInMemory / 0x1000;
		if (sizeInMemory % 0x1000) {
			++numberOfPages;
		}
		printString("  NumberOfPages = 0x");
		printHex(&numberOfPages, sizeof(numberOfPages));
		printString("\n");
		printString("  VirtualMemoryAddress = 0x");
		printHex(&programHeader[i].virtualMemoryAddress, sizeof(programHeader[i].virtualMemoryAddress));
		printString("\n");
		for (size_t j = 0; j < numberOfPages; ++j, memorySeekp += 0x1000) {
			uint64_t virtualMemoryAddress = programHeader[i].virtualMemoryAddress + j * 0x1000;
			virtualMemoryAddress >>= 12;
			uint16_t ptIndex = virtualMemoryAddress & 0x1ff;
			virtualMemoryAddress >>= 9;
			uint16_t pdIndex = virtualMemoryAddress & 0x1ff;
			virtualMemoryAddress >>= 9;
			uint16_t pdptIndex = virtualMemoryAddress & 0x1ff;
			virtualMemoryAddress >>= 9;
			uint16_t pml4tIndex = virtualMemoryAddress & 0x1ff;
			if (pml4t[pml4tIndex].present) {
				PDPTE *pdpt = (PDPTE *)((uint32_t)pml4t[pml4tIndex].pageAddress << 12);
				if (pdpt[pdptIndex].present) {
					PDE *pd = (PDE *)((uint32_t)pdpt[pdptIndex].pageAddress << 12);
					if (pd[pdIndex].present) {
						PTE *pt = (PTE *)((uint32_t)pd[pdIndex].pageAddress << 12);
						if (!pt[ptIndex].present) {
							allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
						}
					} else {
						allocatePagingEntry(&(pd[pdIndex]), newPageStart);
						PTE *pt = (PTE *)newPageStart;
						newPageStart += 0x1000;
						allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
					}
				} else {
					allocatePagingEntry(&(pdpt[pdptIndex]), newPageStart);
					PDE *pd = (PDE *)newPageStart;
					newPageStart += 0x1000;
					allocatePagingEntry(&(pd[pdIndex]), newPageStart);
					PTE *pt = (PTE *)newPageStart;
					newPageStart += 0x1000;
					allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
				}
			} else {
				allocatePagingEntry(&(pml4t[pml4tIndex]), newPageStart);
				PDPTE *pdpt = (PDPTE *)newPageStart;
				newPageStart += 0x1000;
				allocatePagingEntry(&(pdpt[pdptIndex]), newPageStart);
				PDE *pd = (PDE *)newPageStart;
				newPageStart += 0x1000;
				allocatePagingEntry(&(pd[pdIndex]), newPageStart);
				PTE *pt = (PTE *)newPageStart;
				newPageStart += 0x1000;
				allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
			}
		}
	}

	jumpToKernel64(pml4t, infoTableAddress); // Jump to kernel. Code beyond this should never get executed.
	printString("Fatal error : Cannot boot!");
	return 1;
}