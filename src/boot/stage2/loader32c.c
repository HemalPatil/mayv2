#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ACPI3_MemType_Usable 1
#define ACPI3_MemType_Reserved 2
#define ACPI3_MemType_ACPIReclaimable 3
#define ACPI3_MemType_ACPINVS 4
#define ACPI3_MemType_Bad 5
#define ACPI3_MemType_Hole 10

// ACPI 3.0 entry format (we have used extended entries of 24 bytes)
struct ACPI3Entry
{
	uint64_t BaseAddress;
	uint64_t Length;
	uint32_t RegionType;
	uint32_t ExtendedAttributes;
} __attribute__((packed));

// Disk Address Packet for extended disk operations
struct DAP
{
	char DAPSize;
	char alwayszero;
	uint16_t NumberOfSectors;
	uint16_t offset;
	uint16_t segment;
	uint64_t FirstSector;
} __attribute__((packed));
typedef struct DAP DAP;

// Program Header of 64-bit ELF binaries
struct ELF64ProgramHeader
{
	uint32_t TypeOfSegment;
	uint32_t SegmentFlags;
	uint64_t FileOffset;
	uint64_t VirtualMemoryAddress;
	uint64_t Ignore;
	uint64_t SegmentSizeInFile;
	uint64_t SegmentSizeInMemory;
	uint64_t Alignment;
} __attribute__((packed));
typedef struct ELF64ProgramHeader ELF64ProgramHeader;

// 64-bit long mode paging structures
// Right now in this very early stage of system initialization we just need present, r/w and address fields. Hence all the 4 structures are using a common struct
struct PML4E
{
	uint8_t Present : 1;
	uint8_t ReadWrite : 1;
	uint8_t UserAccess : 1;
	uint8_t PageWriteThrough : 1;
	uint8_t CacheDisable : 1;
	uint8_t Accessed : 1;
	uint16_t Ignore1 : 6;
	uint64_t PageAddress : 40;
	uint16_t Ignore2 : 11;
	uint8_t ExecuteDisable : 1;
} __attribute__((packed));
typedef struct PML4E PML4E;
typedef struct PML4E PDPTE;
typedef struct PML4E PDE;
typedef struct PML4E PTE;

char *const vidmem = (char *)0xb8000;
const size_t VGAWidth = 80;
const size_t VGAHeight = 25;
size_t CursorX = 0;
size_t CursorY = 0;
const char endl[] = "\n";
const uint8_t DEFAULT_TERMINAL_COLOR = 0x0f;
uint16_t *InfoTable;

void ClearScreen();
extern void PrintHex(const void *const, const size_t);
extern void swap(void *const, void *const, const size_t);
extern void memset(void *const, const uint8_t, const size_t);
extern void memcopy(const void *const src, void const *dest, const size_t count);
extern void Setup16BitSegments(const uint16_t *const, const void *const, const DAP *const, const uint16_t);
extern void LoadKernelELFSectors();
extern void JumpToKernel(PML4E *, uint16_t *);
extern uint8_t GetLinearAddressLimit();
extern uint8_t GetPhysicalAddressLimit();

void terminalPrintChar(char c, uint8_t color)
{
	if (CursorX >= VGAWidth || CursorY >= VGAHeight)
	{
		return;
	}
	if (c == '\n')
	{
		goto skip;
	}
	const size_t index = 2 * (CursorY * VGAWidth + CursorX);
	vidmem[index] = c;
	vidmem[index + 1] = color;
	if (++CursorX == VGAWidth)
	{
	skip:
		CursorX = 0;
		if (++CursorY == VGAHeight)
		{
			CursorY = 0;
			ClearScreen();
		}
	}
	return;
}

void ClearScreen()
{
	const size_t buffersize = 2 * VGAWidth * VGAHeight;
	for (size_t i = 0; i < buffersize; i = i + 2)
	{
		vidmem[i] = ' ';
		vidmem[i + 1] = DEFAULT_TERMINAL_COLOR;
	}
	CursorX = CursorY = 0;
	return;
}

size_t strlen(const char *const str)
{
	size_t length = 0;
	while (str[length] != 0)
	{
		++length;
	}
	return length;
}

void PrintString(const char *const str)
{
	size_t length = strlen(str);
	for (size_t i = 0; i < length; ++i)
	{
		terminalPrintChar(str[i], DEFAULT_TERMINAL_COLOR);
	}
}

size_t GetNumberOfMMAPEntries()
{
	return (size_t)(*(InfoTable + 1));
}

struct ACPI3Entry *GetMMAPBase()
{
	uint16_t MMAPSegment = *(InfoTable + 3);
	uint16_t MMAPOffset = *(InfoTable + 2);
	return (struct ACPI3Entry *)((uint32_t)MMAPSegment * 16 + (uint32_t)MMAPOffset);
}

void SortMMAPEntries()
{
	// Sort the MMAP entries in ascending order of their base addresses
	// Number of MMAP entries is in the range of 5-15 typically so a simple bubble sort is used

	size_t NumberMMAPEntries = GetNumberOfMMAPEntries();
	struct ACPI3Entry *mmap = GetMMAPBase();
	size_t i, j;
	for (i = 0; i < NumberMMAPEntries - 1; ++i)
	{
		size_t iMin = i;
		for (j = i + 1; j < NumberMMAPEntries; ++j)
		{
			uint64_t basej = *((uint64_t *)(mmap + j));
			uint64_t baseMin = *((uint64_t *)(mmap + iMin));
			if (basej < baseMin)
			{
				iMin = j;
			}
		}
		if (iMin != i)
		{
			swap(mmap + i, mmap + iMin, 24);
		}
	}
}

void ProcessMMAPEntries()
{
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

	size_t NumberMMAPEntries = GetNumberOfMMAPEntries();
	size_t ActualEntries = NumberMMAPEntries;
	struct ACPI3Entry *mmap = GetMMAPBase();
	SortMMAPEntries();
	for (uint32_t i = 0; i < NumberMMAPEntries - 1; ++i)
	{
		struct ACPI3Entry *MMAPEntry1 = mmap + i;
		struct ACPI3Entry *MMAPEntry2 = mmap + i + 1;
		if ((MMAPEntry1->BaseAddress + MMAPEntry1->Length) > MMAPEntry2->BaseAddress)
		{
			if (MMAPEntry1->RegionType == MMAPEntry2->RegionType)
			{
				MMAPEntry1->Length = MMAPEntry2->BaseAddress - MMAPEntry1->BaseAddress;
			}
			else
			{
				if (MMAPEntry1->RegionType == ACPI3_MemType_Usable)
				{
					MMAPEntry1->Length = MMAPEntry2->BaseAddress - MMAPEntry1->BaseAddress;
				}
				else if (MMAPEntry2->RegionType == ACPI3_MemType_Usable)
				{
					MMAPEntry2->BaseAddress = MMAPEntry1->BaseAddress + MMAPEntry1->Length;
				}
			}
		}
		else if ((MMAPEntry1->BaseAddress + MMAPEntry1->Length) < MMAPEntry2->BaseAddress)
		{
			struct ACPI3Entry *NewEntry = mmap + ActualEntries;
			NewEntry->BaseAddress = MMAPEntry1->BaseAddress + MMAPEntry1->Length;
			NewEntry->Length = MMAPEntry2->BaseAddress - NewEntry->BaseAddress;
			NewEntry->RegionType = ACPI3_MemType_Hole;
			NewEntry->ExtendedAttributes = 1;
			++(*(InfoTable + 1));
			++ActualEntries;
		}
	}
	SortMMAPEntries();
}

void IdentityMapFirst16MiB()
{
	// Identity map first 16 MiB of the physical memory
	// page_ptr right now points to PML4T at 0x110000;
	uint64_t *page_ptr = (uint64_t *)0x110000;

	// Clear 22 4KiB pages
	memset(page_ptr, 0, 0xf0 * 4096);

	// PML4T[0] points to PDPT at 0x111000, i.e. the first 512GiB are handled by PDPT at 0x111000
	page_ptr[0] = (uint64_t)0x111003;

	// page_ptr right now points to PDPT at 0x111000 which handles first 512GiB
	page_ptr = (uint64_t *)0x111000;
	// PDPT[0] points to PD at 0x112000, i.e. the first 1GiB is handled by PD at 0x112000
	page_ptr[0] = (uint64_t)0x112003;

	// Each entry in PD points to PT. Each PT handles 2MiB.
	// So we add 8 entries to PD and fill all entries in the PTs starting from 0x113000 to 0x11afff with page_frames 0x0 to 16MiB
	uint64_t *pdentry = (uint64_t *)0x112000;
	uint64_t *ptentry = (uint64_t *)0x113000;
	uint64_t page_frame = (uint64_t)0x0003;
	size_t i = 0, j = 0;
	for (i = 0; i < 8; ++i)
	{
		pdentry[i] = (uint64_t)((uint32_t)ptentry + 3);
		for (j = 0; j < 512; ++j, ++ptentry)
		{
			*(ptentry) = page_frame;
			page_frame += 0x1000;
		}
	}
}

void AllocatePagingEntry(struct PML4E *entry, uint32_t address)
{
	entry->Present = 1;
	entry->ReadWrite = 1;
	entry->PageAddress = address >> 12;
}

int Loader32Main(uint16_t *InfoTableAddress, DAP *const DAPKernel64Address, const void *const LoadModuleAddress)
{
	InfoTable = InfoTableAddress;
	ClearScreen();
	ProcessMMAPEntries();

	// Get the max physical address and max linear address that can be handled by the CPU
	// These details are found by using CPUID.EAX=0x80000008 instruction and has to be done from assembly
	// Refer to Intel documentation Vol. 3 Section 4.1.4
	uint8_t maxphyaddr = GetPhysicalAddressLimit();
	PrintString("Max physical address = 0x");
	PrintHex(&maxphyaddr, sizeof(maxphyaddr));
	PrintString("\n");
	uint8_t maxlinaddr = GetLinearAddressLimit();
	PrintString("Max linear address = 0x");
	PrintHex(&maxlinaddr, sizeof(maxlinaddr));
	PrintString("\n");
	*(InfoTable + 9) = (uint16_t)maxphyaddr;
	*(InfoTable + 10) = (uint16_t)maxlinaddr;

	DAP DAPKernel64 = *DAPKernel64Address;
	uint16_t KernelNumberOfSectors = DAPKernel64.NumberOfSectors;
	uint32_t bytesOfKernelELF = (uint32_t)0x800 * (uint32_t)KernelNumberOfSectors;
	uint64_t KernelVirtualMemSize = *((uint64_t *)(InfoTable + 0xc)); // Get size of kernel in virtual memory

	// Check if enough space is available to load kernel as well as the elf (i.e. length of region > (KernelVirtualMemSize + bytesOfKernelELF))
	// We will load parsed kernel code from 2MiB physical memory (size : KernelVirtualMemSize)
	// Kernel ELF will be loaded at 2MiB + KernelVirtualMemSize + 4KiB physical memory form where it will be parsed
	bool enoughSpace = false;
	size_t numberMMAPentries = GetNumberOfMMAPEntries();
	PrintString("Number of MMAP entries = 0x");
	PrintHex(&numberMMAPentries, sizeof(numberMMAPentries));
	PrintString("\n");
	struct ACPI3Entry *mmap = GetMMAPBase();
	for (size_t i = 0; i < numberMMAPentries; ++i)
	{
		if ((mmap[i].BaseAddress <= (uint64_t)0x100000) && (mmap[i].Length > ((uint64_t)0x201000 - mmap[i].BaseAddress + (uint64_t)bytesOfKernelELF + KernelVirtualMemSize)))
		{
			enoughSpace = true;
			break;
		}
	}
	if (!enoughSpace)
	{
		ClearScreen();
		PrintString("\nFatal Error : System memory is fragmented too much.\nNot enough space to load kernel.\nCannot boot!");
		return 1;
	}
	PrintString("Loading kernel...\n");

	// We will be identity mapping the first 16 MiB of the physical memory
	// To see how the mapping is done refer to docs/mapping.txt
	IdentityMapFirst16MiB();

	// In GDT change base address of the 16-bit segments
	Setup16BitSegments(InfoTableAddress, LoadModuleAddress, DAPKernel64Address, *InfoTable); // InfoTable[0] = boot disk number

	// Enter the kernel physical memory base address in the info table
	uint32_t KernelBase = 0x200000;
	*((uint64_t *)(InfoTable + 0x10)) = (uint64_t)KernelBase;

	// We have 64 KiB free in physical memory from 0x80000 to 0x90000. The sector in our OS ISO image is 2 KiB in size.
	// So we can load the kernel ELF in batches of 32 sectors. Leave a gap of 4 KiB between kernel process and kernel ELF
	uint32_t KernelELFBase = 0x201000 + (uint32_t)KernelVirtualMemSize;
	PrintString("KernelVirtualMemSize = 0x");
	PrintHex(&KernelVirtualMemSize, sizeof(KernelVirtualMemSize));
	PrintString("\n");
	PrintString("KernelELFBase = 0x");
	PrintHex(&KernelELFBase, sizeof(KernelELFBase));
	PrintString("\n");
	DAPKernel64Address->offset = 0x0;
	DAPKernel64Address->segment = 0x8000;
	DAPKernel64Address->NumberOfSectors = 32;
	uint16_t iters = KernelNumberOfSectors / 32;
	memset((void *)0x80000, 0, 0x10000);
	for (uint16_t i = 0; i < iters; ++i)
	{
		LoadKernelELFSectors();
		memcopy((void *)0x80000, (void *)(KernelELFBase + i * 0x10000), 0x10000);
		DAPKernel64Address->FirstSector += 32;
	}
	// Load remaining sectors
	DAPKernel64Address->NumberOfSectors = KernelNumberOfSectors % 32;
	LoadKernelELFSectors();
	memcopy((void *)0x80000, (void *)(KernelELFBase + iters * 0x10000), DAPKernel64Address->NumberOfSectors * 0x800);

	PrintString("Kernel executable loaded.\n");

	// Parse the kernel executable
	uint16_t ELFFlags = *((uint16_t *)(KernelELFBase + 4));
	if (ELFFlags != 0x0102) // Make sure kernel executable is 64-bit in little endian format
	{
		PrintString("\nKernel executable corrupted! Cannot boot!");
		return 1;
	}
	uint32_t ProgramHeaderTable = *((uint32_t *)(KernelELFBase + 32));
	uint16_t ProgramHeaderEntrySize = *((uint16_t *)(KernelELFBase + 54));
	uint16_t ProgramHeaderEntryCount = *((uint16_t *)(KernelELFBase + 56));
	if (ProgramHeaderEntrySize != sizeof(ELF64ProgramHeader))
	{
		PrintString("\nKernel executable corrupted! Cannot boot!");
		return 1;
	}
	ELF64ProgramHeader *ProgramHeader = (ELF64ProgramHeader *)(KernelELFBase + ProgramHeaderTable);
	uint32_t MemorySeekp = KernelBase;
	PrintString("KernelBase = 0x");
	PrintHex(&KernelBase, sizeof(KernelBase));
	PrintString("\n");
	PrintString("ProgramHeaderEntryCount = 0x");
	PrintHex(&ProgramHeaderEntryCount, sizeof(ProgramHeaderEntryCount));
	PrintString("\n");
	PML4E *PML4T = (PML4E *)0x110000;
	uint32_t NewPageStart = 0x11b000; // New pages that need to be made should start from this address and add 0x1000 to it.
	for (uint16_t i = 0; i < ProgramHeaderEntryCount; ++i)
	{
		uint32_t SizeInMemory = (uint32_t)ProgramHeader[i].SegmentSizeInMemory;
		PrintString("  SizeInMemory = 0x");
		PrintHex(&SizeInMemory, sizeof(SizeInMemory));
		PrintString("\n");

		memset((void *)MemorySeekp, 0, SizeInMemory);
		memcopy((void *)(KernelELFBase + (uint32_t)ProgramHeader[i].FileOffset), (void *)MemorySeekp, (uint32_t)ProgramHeader[i].SegmentSizeInFile);

		// Our kernel is linked at higher half addresses. (right now it is last 2GiB of 64-bit address space, may change if linker script is changed)
		// Map this section in the paging structure
		size_t NumberOfPages = SizeInMemory / 0x1000;
		if (SizeInMemory % 0x1000)
		{
			++NumberOfPages;
		}
		PrintString("  NumberOfPages = 0x");
		PrintHex(&NumberOfPages, sizeof(NumberOfPages));
		PrintString("\n");
		PrintString("  VirtualMemoryAddress = 0x");
		PrintHex(&ProgramHeader[i].VirtualMemoryAddress, sizeof(ProgramHeader[i].VirtualMemoryAddress));
		PrintString("\n");
		for (size_t j = 0; j < NumberOfPages; ++j, MemorySeekp += 0x1000)
		{
			uint64_t VirtualMemoryAddress = ProgramHeader[i].VirtualMemoryAddress + j * 0x1000;
			VirtualMemoryAddress >>= 12;
			uint16_t PTIndex = VirtualMemoryAddress & 0x1ff;
			VirtualMemoryAddress >>= 9;
			uint16_t PDIndex = VirtualMemoryAddress & 0x1ff;
			VirtualMemoryAddress >>= 9;
			uint16_t PDPTIndex = VirtualMemoryAddress & 0x1ff;
			VirtualMemoryAddress >>= 9;
			uint16_t PML4TIndex = VirtualMemoryAddress & 0x1ff;
			if (PML4T[PML4TIndex].Present)
			{
				PDPTE *PDPT = (PDPTE *)((uint32_t)PML4T[PML4TIndex].PageAddress << 12);
				if (PDPT[PDPTIndex].Present)
				{
					PDE *PD = (PDE *)((uint32_t)PDPT[PDPTIndex].PageAddress << 12);
					if (PD[PDIndex].Present)
					{
						PTE *PT = (PTE *)((uint32_t)PD[PDIndex].PageAddress << 12);
						if (!PT[PTIndex].Present)
						{
							AllocatePagingEntry(&(PT[PTIndex]), MemorySeekp);
						}
					}
					else
					{
						AllocatePagingEntry(&(PD[PDIndex]), NewPageStart);
						PTE *PT = (PTE *)NewPageStart;
						NewPageStart += 0x1000;
						AllocatePagingEntry(&(PT[PTIndex]), MemorySeekp);
					}
				}
				else
				{
					AllocatePagingEntry(&(PDPT[PDPTIndex]), NewPageStart);
					PDE *PD = (PDE *)NewPageStart;
					NewPageStart += 0x1000;
					AllocatePagingEntry(&(PD[PDIndex]), NewPageStart);
					PTE *PT = (PTE *)NewPageStart;
					NewPageStart += 0x1000;
					AllocatePagingEntry(&(PT[PTIndex]), MemorySeekp);
				}
			}
			else
			{
				AllocatePagingEntry(&(PML4T[PML4TIndex]), NewPageStart);
				PDPTE *PDPT = (PDPTE *)NewPageStart;
				NewPageStart += 0x1000;
				AllocatePagingEntry(&(PDPT[PDPTIndex]), NewPageStart);
				PDE *PD = (PDE *)NewPageStart;
				NewPageStart += 0x1000;
				AllocatePagingEntry(&(PD[PDIndex]), NewPageStart);
				PTE *PT = (PTE *)NewPageStart;
				NewPageStart += 0x1000;
				AllocatePagingEntry(&(PT[PTIndex]), MemorySeekp);
			}
		}
	}

	JumpToKernel(PML4T, InfoTableAddress); // Jump to kernel. Code beyond this should never get executed.
	// ClearScreen();
	PrintString("Fatal error : Cannot boot!");
	return 1;
}