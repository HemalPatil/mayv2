#include <acpi.h>
#include <elf64.h>
#include <infotable.h>
#include <pml4t.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ISO_SECTOR_SIZE 2048
#define KERNEL_HIGHERHALF_ORIGIN 0xffffffff80000000
#define L32_IDENTITY_MAP_SIZE 32
#define L32K64_SCRATCH_BASE 0x80000
#define L32K64_SCRATCH_LENGTH 0x10000
#define LOADER32_ORIGIN 0x00100000
#define PML4_PAGE_PRESENT_READ_WRITE 3

// Disk Address Packet for extended disk operations
struct DAP {
	char dapSize;
	char alwayszero;
	uint16_t sectorCount;
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
const uint8_t pageSizeShift = 12;
const uint64_t pageSize = 1 << pageSizeShift;
const uint64_t pageSizeMask = ~(((uint64_t)1 << pageSizeShift) - 1);
const size_t virtualPageIndexShift = 9;
const uint64_t virtualPageIndexMask = ((uint64_t)1 << virtualPageIndexShift) - 1;
InfoTable *infoTable;

void clearScreen();
extern void printHex(const void *const, const size_t);
extern void swap(void *const a, void *const b, const size_t);
extern void memset(void *const dest, const uint8_t, const size_t);
extern void memcpy(const void *const src, void const *dest, const size_t count);
extern void setup16BitSegments(const InfoTable* const table, const void *const loadModule, const DAP* const dap, const uint16_t);
extern void loadKernel64ElfSectors();
extern void jumpToKernel64(
	const PML4E* const pml4t,
	const InfoTable* const table,
	const uint32_t lowerHalfSize,
	const uint32_t higherHalfSize,
	const uint32_t usablePhyMemStart
);
extern uint8_t getLinearAddressLimit();
extern uint8_t getPhysicalAddressLimit();

void terminalPrintChar(char c, uint8_t color) {
	if (cursorX >= vgaWidth || cursorY >= vgaHeight) {
		return;
	}
	if (c != '\n') {
		const size_t index = 2 * (cursorY * vgaWidth + cursorX);
		vidMem[index] = c;
		vidMem[index + 1] = color;
	}
	if (++cursorX == vgaWidth || c == '\n') {
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
	for (int i = 0; i < infoTable->mmapEntryCount - 1; ++i) {
		int iMin = i;
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
	// 2) Regions are of different types and one of them is of type ACPI3_MEM_TYPE_USABLE
	//		- Change the length of the first entry if it is usable
	//		- Change the base of the second entry if it is usable
	// 3) Overlapping of two regions of different types and none of them of type ACPI3_MEM_TYPE_USABLE
	//    is not handled right now.

	ACPI3Entry *mmap = getMmapBase();
	size_t actualEntries = infoTable->mmapEntryCount;
	sortMmapEntries();
	for (int i = 0; i < infoTable->mmapEntryCount - 1; ++i) {
		ACPI3Entry *mmapEntry1 = mmap + i;
		ACPI3Entry *mmapEntry2 = mmap + i + 1;
		if ((mmapEntry1->base + mmapEntry1->length) > mmapEntry2->base) {
			if (mmapEntry1->regionType == mmapEntry2->regionType) {
				mmapEntry1->length = mmapEntry2->base - mmapEntry1->base;
			} else {
				if (mmapEntry1->regionType == ACPI3_MEM_TYPE_USABLE) {
					mmapEntry1->length = mmapEntry2->base - mmapEntry1->base;
				} else if (mmapEntry2->regionType == ACPI3_MEM_TYPE_USABLE) {
					mmapEntry2->base = mmapEntry1->base + mmapEntry1->length;
				}
			}
		} else if ((mmapEntry1->base + mmapEntry1->length) < mmapEntry2->base) {
			ACPI3Entry *newEntry = mmap + actualEntries;
			newEntry->base = mmapEntry1->base + mmapEntry1->length;
			newEntry->length = mmapEntry2->base - newEntry->base;
			newEntry->regionType = ACPI3_MEM_TYPE_HOLE;
			newEntry->extendedAttributes = 1;
			++actualEntries;
		}
	}
	infoTable->mmapEntryCount = actualEntries;
	sortMmapEntries();
}

void identityMapMemory(uint64_t* pagePtr) {
	printString("PML4E root = 0x");
	printHex(&pagePtr, sizeof(pagePtr));
	printString("\n");
	// Identity map first L32_IDENTITY_MAP_SIZE MiB of the physical memory
	// L32_IDENTITY_MAP_SIZE will never exceed 256MiB
	// 1 PML4T page - entry 0 maps first 512 GiB
	// 1 PDPT page - entry 0 maps first 1GiB
	// 1 PDT page - entries 0 to L32_IDENTITY_MAP_SIZE / 2 map first L32_IDENTITY_MAP_SIZE MiB
	//				since each PT maps 2MiB
	// L32_IDENTITY_MAP_SIZE / 2 PT pages
	memset(pagePtr, 0, (3 + L32_IDENTITY_MAP_SIZE / 2) * pageSize);

	// PML4T[0] points to PDPT at pml4Root + pageSize, i.e. handles the first 512GiB
	pagePtr[0] = (infoTable->pml4tPhysicalAddress + pageSize) | PML4_PAGE_PRESENT_READ_WRITE;

	// pagePtr right now points to PDPT which handles first 512GiB
	pagePtr = (uint64_t *)(uint32_t)(infoTable->pml4tPhysicalAddress + pageSize);
	// PDPT[0] points to PD which handles the first 1GiB
	pagePtr[0] = (infoTable->pml4tPhysicalAddress + 2 * pageSize) | PML4_PAGE_PRESENT_READ_WRITE;

	// Each entry in PD points to PT. Each PT handles 2MiB.
	// Add L32_IDENTITY_MAP_SIZE / 2 entries to PD
	// and fill all entries in the PTs with page frames 0x0 to L32_IDENTITY_MAP_SIZE MiB
	uint64_t *pdEntry = (uint64_t *)(uint32_t)(infoTable->pml4tPhysicalAddress + 2 * pageSize);
	uint64_t *ptEntry = (uint64_t *)(uint32_t)(infoTable->pml4tPhysicalAddress + 3 * pageSize);
	uint64_t pageFrame = 0x0 | PML4_PAGE_PRESENT_READ_WRITE;
	size_t i = 0, j = 0;
	for (i = 0; i < L32_IDENTITY_MAP_SIZE / 2; ++i) {
		pdEntry[i] = (uint32_t)ptEntry | PML4_PAGE_PRESENT_READ_WRITE;
		for (j = 0; j < 512; ++j, ++ptEntry) {
			*(ptEntry) = pageFrame;
			pageFrame += pageSize;
		}
	}
}

void allocatePagingEntry(PML4E *entry, uint32_t address) {
	entry->present = 1;
	entry->readWrite = 1;
	entry->physicalAddress = address >> pageSizeShift;
}

int loader32Main(uint32_t loader32VirtualMemSize, InfoTable *infoTableAddress, DAP *const dapKernel64, const void *const loadModuleAddress) {
	infoTable = infoTableAddress;
	clearScreen();
	printString("Loader32 virtual memory size = 0x");
	printHex(&loader32VirtualMemSize, sizeof(loader32VirtualMemSize));
	printString("\n");

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

	uint16_t kernelSectorCount = dapKernel64->sectorCount;
	uint32_t kernelElfSize = ISO_SECTOR_SIZE * kernelSectorCount;
	uint64_t kernelVirtualMemSize = 0;

	// Change base address of the 16-bit segments in GDT32
	setup16BitSegments(infoTableAddress, loadModuleAddress, dapKernel64, infoTable->bootDiskNumber);

	// Load first 32 sectors of kernel ELF and find its virtual memory size
	const uint32_t sectors32 = L32K64_SCRATCH_LENGTH / ISO_SECTOR_SIZE;
	dapKernel64->offset = 0x0;
	dapKernel64->segment = L32K64_SCRATCH_BASE >> 4;
	dapKernel64->sectorCount = sectors32;
	memset((void*)L32K64_SCRATCH_BASE, 0, L32K64_SCRATCH_LENGTH);
	loadKernel64ElfSectors();
	ELF64Header *elfHeader = (ELF64Header*)L32K64_SCRATCH_BASE;
	if (
		*((uint32_t*)(&elfHeader->signature)) != ELF_SIGNATURE ||
		elfHeader->endianness != ELF_LITTLE_ENDIAN ||
		elfHeader->isX64 != ELF_TYPE_64
	) {
		printString("\nKernel executable corrupted (ELF header incorrect)! Cannot boot!");
		return 1;
	}
	if (elfHeader->headerEntrySize != sizeof(ELF64ProgramHeader)) {
		printString("\nKernel executable corrupted (header entry size mismatch)! Cannot boot!");
		return 1;
	}
	ELF64ProgramHeader *programHeader = (ELF64ProgramHeader *)((uint32_t)elfHeader + (uint32_t)elfHeader->headerTablePosition);
	for (uint16_t i = 0; i < elfHeader->headerEntryCount; ++i) {
		if (programHeader[i].segmentType != ELF_SEGMENT_TYPE_LOAD) {
			continue;
		}
		// Align size to 4KiB page boundary
		uint64_t size = programHeader[i].segmentSizeInMemory & pageSizeMask;
		if (size != programHeader[i].segmentSizeInMemory) {
			size += pageSize;
		}
		kernelVirtualMemSize += size;
	}
	printString("KernelVirtualMemSize = 0x");
	printHex(&kernelVirtualMemSize, sizeof(kernelVirtualMemSize));
	printString("\n");

	// Check if enough space is available to load kernel process, kernel ELF
	// (i.e. base address < (LOADER32_ORIGIN + loader 32 size)
	// and length of region > (kernelVirtualMemSize + kernelElfSize))
	bool enoughSpace = false;
	printString("Number of MMAP entries = 0x");
	printHex(&infoTable->mmapEntryCount, sizeof(infoTable->mmapEntryCount));
	printString("\n");
	ACPI3Entry *mmap = getMmapBase();
	for (size_t i = 0; i < infoTable->mmapEntryCount; ++i) {
		if (
			(mmap[i].base <= ((uint64_t)LOADER32_ORIGIN + loader32VirtualMemSize)) &&
			(mmap[i].length >= (kernelElfSize + kernelVirtualMemSize))
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

	// Enter the kernel physical memory base address in the info table
	uint32_t kernelBase = LOADER32_ORIGIN + loader32VirtualMemSize;
	infoTable->kernelPhyMemBase = (uint64_t)kernelBase;
	uint32_t kernelElfBase = kernelBase + (uint32_t)kernelVirtualMemSize;
	printString("KernelELFBase = 0x");
	printHex(&kernelElfBase, sizeof(kernelElfBase));
	printString("\n");
	printString("KernelELFSize = 0x");
	printHex(&kernelElfSize, sizeof(kernelElfSize));
	printString("\n");

	// First 32 sectors of kernel are already loaded. Copy them to ELF base.
	memcpy((void*)L32K64_SCRATCH_BASE, (void*)kernelElfBase, L32K64_SCRATCH_LENGTH);

	// Load rest of the kernel ELF
	uint16_t iters = kernelSectorCount / sectors32;
	dapKernel64->firstSector += sectors32;
	for (uint16_t i = 1; i < iters; ++i) {
		memset((void*)L32K64_SCRATCH_BASE, 0, L32K64_SCRATCH_LENGTH);
		loadKernel64ElfSectors();
		memcpy((void*)L32K64_SCRATCH_BASE, (void *)(kernelElfBase + i * L32K64_SCRATCH_LENGTH), L32K64_SCRATCH_LENGTH);
		dapKernel64->firstSector += sectors32;
	}
	// Load remaining sectors
	dapKernel64->sectorCount = kernelSectorCount % sectors32;
	loadKernel64ElfSectors();
	memcpy((void *)L32K64_SCRATCH_BASE, (void *)(kernelElfBase + iters * L32K64_SCRATCH_LENGTH), dapKernel64->sectorCount * ISO_SECTOR_SIZE);

	printString("Kernel executable loaded.\n");

	size_t pml4Count = 3 + L32_IDENTITY_MAP_SIZE / 2; // Look at identityMapMemory comments to understand how this is calculated
	// Align PML4 root to 4 KiB boundary
	infoTable->pml4tPhysicalAddress = (kernelElfBase + kernelElfSize) & pageSizeMask;
	if (infoTable->pml4tPhysicalAddress != kernelElfBase + kernelElfSize) {
		infoTable->pml4tPhysicalAddress += pageSize;
	}
	identityMapMemory((uint64_t*)(uint32_t)infoTable->pml4tPhysicalAddress);
	uint32_t memorySeekp = kernelBase;
	printString("KernelBase = 0x");
	printHex(&kernelBase, sizeof(kernelBase));
	printString("\n");
	printString("ProgramHeaderEntryCount = 0x");
	printHex(&elfHeader->headerEntryCount, sizeof(elfHeader->headerEntryCount));
	printString("\n");
	PML4E *pml4t = (PML4E *)(uint32_t)infoTable->pml4tPhysicalAddress;
	// New pages that need to be made should start from this address and add pageSize to it.
	uint32_t newPageStart = infoTable->pml4tPhysicalAddress + pml4Count * pageSize;
	uint32_t lowerHalfSize = 0, higherHalfSize = 0;
	elfHeader = (ELF64Header*)kernelElfBase;
	programHeader = (ELF64ProgramHeader*)(kernelElfBase + (uint32_t)elfHeader->headerTablePosition);
	for (uint16_t i = 0; i < elfHeader->headerEntryCount; ++i) {
		if (programHeader[i].segmentType != ELF_SEGMENT_TYPE_LOAD) {
			continue;
		}
		uint32_t sizeInMemory = (uint32_t)programHeader[i].segmentSizeInMemory;
		printString("  SizeInMemory = 0x");
		printHex(&sizeInMemory, sizeof(sizeInMemory));
		printString("\n");

		memset((void *)memorySeekp, 0, sizeInMemory);
		memcpy((void *)(kernelElfBase + (uint32_t)programHeader[i].fileOffset), (void *)memorySeekp, (uint32_t)programHeader[i].segmentSizeInFile);

		// Kernel is linked at higher half addresses.
		// Right now it is last 2GiB of 64-bit address space, may change if linker script is changed
		// Map this section in the paging structure
		size_t pageCount = sizeInMemory / pageSize;
		if (sizeInMemory - pageCount * pageSize) {
			++pageCount;
		}
		if (programHeader[i].virtualAddress < (uint64_t)KERNEL_HIGHERHALF_ORIGIN) {
			lowerHalfSize += pageCount * pageSize;
		} else {
			higherHalfSize += pageCount * pageSize;
		}
		printString("  PageCount = 0x");
		printHex(&pageCount, sizeof(pageCount));
		printString("\n");
		printString("  VirtualAddress = 0x");
		printHex(&programHeader[i].virtualAddress, sizeof(programHeader[i].virtualAddress));
		printString("\n");
		for (size_t j = 0; j < pageCount; ++j, memorySeekp += pageSize) {
			uint64_t virtualAddress = programHeader[i].virtualAddress + j * pageSize;
			virtualAddress >>= pageSizeShift;
			size_t ptIndex = virtualAddress & virtualPageIndexMask;
			virtualAddress >>= virtualPageIndexShift;
			size_t pdIndex = virtualAddress & virtualPageIndexMask;
			virtualAddress >>= virtualPageIndexShift;
			size_t pdptIndex = virtualAddress & virtualPageIndexMask;
			virtualAddress >>= virtualPageIndexShift;
			size_t pml4tIndex = virtualAddress & virtualPageIndexMask;
			if (pml4t[pml4tIndex].present) {
				PDPTE *pdpt = (PDPTE *)((uint32_t)pml4t[pml4tIndex].physicalAddress << pageSizeShift);
				if (pdpt[pdptIndex].present) {
					PDE *pd = (PDE *)((uint32_t)pdpt[pdptIndex].physicalAddress << pageSizeShift);
					if (pd[pdIndex].present) {
						PTE *pt = (PTE *)((uint32_t)pd[pdIndex].physicalAddress << pageSizeShift);
						if (!pt[ptIndex].present) {
							allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
						}
					} else {
						allocatePagingEntry(&(pd[pdIndex]), newPageStart);
						PTE *pt = (PTE *)newPageStart;
						newPageStart += pageSize;
						allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
					}
				} else {
					allocatePagingEntry(&(pdpt[pdptIndex]), newPageStart);
					PDE *pd = (PDE *)newPageStart;
					newPageStart += pageSize;
					allocatePagingEntry(&(pd[pdIndex]), newPageStart);
					PTE *pt = (PTE *)newPageStart;
					newPageStart += pageSize;
					allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
				}
			} else {
				allocatePagingEntry(&(pml4t[pml4tIndex]), newPageStart);
				PDPTE *pdpt = (PDPTE *)newPageStart;
				newPageStart += pageSize;
				allocatePagingEntry(&(pdpt[pdptIndex]), newPageStart);
				PDE *pd = (PDE *)newPageStart;
				newPageStart += pageSize;
				allocatePagingEntry(&(pd[pdIndex]), newPageStart);
				PTE *pt = (PTE *)newPageStart;
				newPageStart += pageSize;
				allocatePagingEntry(&(pt[ptIndex]), memorySeekp);
			}
		}
	}

	// Jump to kernel. Code beyond this should never get executed.
	jumpToKernel64(pml4t, infoTableAddress, lowerHalfSize, higherHalfSize, newPageStart);
	printString("Fatal error : Cannot boot!");
	return 1;
}