#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>

#define ACPI3_MemType_Usable 1
#define ACPI3_MemType_Reserved 2
#define ACPI3_MemType_ACPIReclaimable 3
#define ACPI3_MemType_ACPINVS 4
#define ACPI3_MemType_Bad 5
#define ACPI3_MemType_Hole 10

struct ACPI3Entry
{
	uint64_t BaseAddress;
	uint64_t Length;
	uint32_t RegionType;
	uint32_t ExtendedAttributes;
} __attribute__((packed));

struct DAP //Disk Address Packet
{
	char DAPSize;
	char alwayszero;
	uint16_t NumberOfSectors;
	uint16_t offset;
	uint16_t segment;
	uint64_t FirstSector;
} __attribute__((packed));
typedef struct DAP DAP;

struct PML4E
{
	unsigned int present:1;
	unsigned int rw:1;
	unsigned int user_supervisor:1;
	unsigned int pwt:1;
	unsigned int pcd:1;
	unsigned int accessed:1;
	unsigned int ignore1:1;
	unsigned int page_size:1;
	unsigned int ignore2:4;
	unsigned int pdpt_addr:27;
	unsigned int reserved:13;
	unsigned int ignore3:11;
	unsigned int xd:1;
} __attribute__((packed));

typedef struct PML4E PML4E;
typedef PML4E PDPTE;
typedef PML4E PDE;

struct PTE
{
	unsigned int present:1;
	unsigned int rw:1;
	unsigned int user_supervisor:1;
	unsigned int pwt:1;
	unsigned int pcd:1;
	unsigned int accessed:1;
	unsigned int dirty:1;
	unsigned int pat:1;
	unsigned int global:1;
	unsigned int ignore2:3;
	unsigned int pdpt_addr:27;
	unsigned int reserved:13;
	unsigned int ignore3:11;
	unsigned int xd:1;
} __attribute__((packed));

typedef struct PTE PTE;

struct PML4
{
	PML4E pml4entries[512];
} __attribute__((packed));
typedef struct PML4 PML4;

struct PDPT
{
	PDPTE pdptentries[512];
} __attribute__((packed));
typedef struct PDPT PDPT;

struct PD
{
	PDE pdentries[512];
} __attribute__((packed));
typedef struct PD PD;

struct PT
{
	PTE ptentries[512];
} __attribute__((packed));
typedef struct PT PT;

char* const vidmem = (char*) 0xb8000;
const size_t VGAWidth = 80;
const size_t VGAHeight = 25;
size_t CursorX = 0;
size_t CursorY = 0;
const char endl[] = "\n";
const uint8_t DEFAULT_TERMINAL_COLOR = 0x0f;
uint16_t* InfoTable;

void ClearScreen();
extern void PrintHex(const void* const, size_t);
extern void swap(void*, void*, size_t);
extern void memset(void*, uint8_t, size_t);
extern void memcopy(const void* const src, void const* dest, size_t count);
extern void Setup16BitSegments(uint16_t*, const void* const);
extern void LoadKernelELFSectors();
extern uint8_t GetLinearAddressLimit();
extern uint8_t GetPhysicalAddressLimit();

void TerminalPutChar(char c, uint8_t color)
{
	if(CursorX>=VGAWidth || CursorY>=VGAHeight)
	{
		return;
	}
	if(c=='\n')
	{
		goto skip;
	}
	const size_t index = 2*(CursorY*VGAWidth + CursorX);
	vidmem[index] = c;
	vidmem[index+1] = color;
	if(++CursorX == VGAWidth)
	{
		skip:
		CursorX = 0;
		if(++CursorY == VGAHeight)
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
	for(size_t i=0; i<buffersize; i=i+2)
	{
		vidmem[i] = ' ';
		vidmem[i+1] = DEFAULT_TERMINAL_COLOR;
	}
	CursorX = CursorY = 0;
	return;
}

size_t strlen(const char* const str)
{
	size_t length = 0;
	while(str[length] != 0)
	{
		length++;
	}
	return length;
}

void PrintString(const char* const str)
{
	size_t length = strlen(str);
	for(size_t i=0;i<length;i++)
	{
		TerminalPutChar(str[i], DEFAULT_TERMINAL_COLOR);
	}
}

size_t GetNumberOfMMAPEntries()
{
	return (size_t)(*(InfoTable + 1));
}

struct ACPI3Entry* GetMMAPBase()
{
	uint16_t MMAPSegment = *(InfoTable + 3);
	uint16_t MMAPOffset = *(InfoTable + 2);
	return (struct ACPI3Entry*)((uint32_t)MMAPSegment*16 + (uint32_t)MMAPOffset);
}

void SortMMAPEntries()
{
	// Sort the MMAP entries in ascending order of their base addresses
	// Number of MMAP entries is in the range of 5-15 typically so a simple bubble sort is used

	size_t NumberMMAPEntries = GetNumberOfMMAPEntries();
	struct ACPI3Entry *mmap = GetMMAPBase();
	size_t i,j;
	for(i=0; i<NumberMMAPEntries - 1; i++)
	{
		size_t iMin = i;
		for(j=i+1; j<NumberMMAPEntries; j++)
		{
			uint64_t basej = *((uint64_t*)(mmap + j));
			uint64_t baseMin = *((uint64_t*)(mmap + iMin));
			if(basej < baseMin)
			{
				iMin = j;
			}
		}
		if(iMin != i)
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
	for(uint32_t i=0; i<NumberMMAPEntries-1;i++)
	{
		struct ACPI3Entry *MMAPEntry1 = mmap + i;
		struct ACPI3Entry *MMAPEntry2 = mmap + i + 1;
		if((MMAPEntry1->BaseAddress + MMAPEntry1->Length) > MMAPEntry2->BaseAddress)
		{
			if(MMAPEntry1->RegionType == MMAPEntry2->RegionType)
			{
				MMAPEntry1->Length = MMAPEntry2->BaseAddress - MMAPEntry1->BaseAddress;
			}
			else
			{
				if(MMAPEntry1->RegionType == ACPI3_MemType_Usable)
				{
					MMAPEntry1->Length = MMAPEntry2->BaseAddress - MMAPEntry1->BaseAddress;
				}
				else if(MMAPEntry2->RegionType == ACPI3_MemType_Usable)
				{
					MMAPEntry2->BaseAddress = MMAPEntry1->BaseAddress + MMAPEntry1->Length;
				}
			}
		}
		else if((MMAPEntry1->BaseAddress + MMAPEntry1->Length) < MMAPEntry2->BaseAddress)
		{
			struct ACPI3Entry *NewEntry = mmap + ActualEntries;
			NewEntry->BaseAddress = MMAPEntry1->BaseAddress + MMAPEntry1->Length;
			NewEntry->Length = MMAPEntry2->BaseAddress - NewEntry->BaseAddress;
			NewEntry->RegionType = ACPI3_MemType_Hole;
			NewEntry->ExtendedAttributes = 1;
			(*(InfoTable + 1))++;
			ActualEntries++;
		}
	}
	SortMMAPEntries();
}

uint64_t GetRAMSize()
{
	size_t NumberMMAPEntries = GetNumberOfMMAPEntries();
	struct ACPI3Entry* MMAPLastEntry = GetMMAPBase() + NumberMMAPEntries - 1;
	return MMAPLastEntry->BaseAddress + MMAPLastEntry->Length;
}

uint64_t GetUsableRAMSize()
{
	size_t NumberMMAPEntries = GetNumberOfMMAPEntries();
	struct ACPI3Entry* MMAPEntry = GetMMAPBase();
	uint64_t UsableRAMSize = 0;
	for(size_t i=0;i<NumberMMAPEntries;i++)
	{
		if(MMAPEntry->RegionType == ACPI3_MemType_Usable)
		{
			UsableRAMSize += MMAPEntry->Length;
		}
		MMAPEntry++;
	}
	return UsableRAMSize;
}

void SetupPAEPagingLongMode()
{
	// Identity map first 16 MiB of the physical memory
	uint64_t *page_ptr = (uint64_t*)0x110000;
	memset(page_ptr, 0, 11*4096);
	*(page_ptr) = (uint64_t)0x111003;
	page_ptr = (uint64_t*)0x111000;
	*(page_ptr) = (uint64_t)0x112003;
	uint64_t *pdentry = (uint64_t*)0x112000;
	uint64_t *ptentry = (uint64_t*)0x113000;
	uint64_t page_frame = (uint64_t)0x0003;
	size_t i=0,j=0;
	for(i=0;i<8;i++)
	{
		pdentry[i] = (((uint64_t)(uint32_t)ptentry) + 3);
		for(j=0;j<512;j++)
		{
			*(ptentry) = page_frame;
			page_frame += 0x1000;
			ptentry++;
		}
	}
}

int Loader32Main(uint16_t* InfoTableAddress, DAP* const DAPKernel64Address, const void* const LoadModuleAddress)
{
	InfoTable = InfoTableAddress;
	ClearScreen();
	ProcessMMAPEntries();

	uint64_t RAMSize = GetRAMSize();
	uint64_t UsableRAMSize = GetUsableRAMSize();

	PrintString("Loader 32-bit, written in C\n");
	PrintString("RAM size : ");
	PrintHex(&RAMSize, 8);
	PrintString(endl);
	PrintString("Usable RAM size : ");
	PrintHex(&UsableRAMSize, 8);
	PrintString(endl);

	// Get the max physical address and max linear address that can be handled by the CPU
	// These details are found by using CPUID.EAX=0x80000008 instruction and has to be done from assembly
	// Refer to Intel documentation Vol. 3 Section 4.1.4 
	uint8_t maxphyaddr = GetPhysicalAddressLimit();
	uint8_t maxlinaddr = GetLinearAddressLimit();
	*(InfoTable + 9) = (uint16_t)maxphyaddr;
	*(InfoTable + 10) = (uint16_t)maxlinaddr;
	PrintString("Max phy addr limit : ");
	PrintHex(&maxphyaddr,1);
	PrintString("\nMax Linear addr limit : ");
	PrintHex(&maxlinaddr,1);
	PrintString(endl);

	PrintString("Info Table Address : ");
	PrintHex(&InfoTable, 4);
	PrintString("\nkernel64 DAP : ");
	PrintHex(&DAPKernel64Address, 4);
	PrintString("\nLoad module address : ");
	PrintHex(&LoadModuleAddress, 4);

	DAP DAPKernel64 = *DAPKernel64Address;
	PrintString("\nNumber of sectors of kernel : ");
	uint16_t KernelNumberOfSectors = DAPKernel64.NumberOfSectors;
	PrintHex(&KernelNumberOfSectors, 2);
	uint32_t bytesOfKernelELF = (uint32_t)0x800 * (uint32_t)KernelNumberOfSectors;
	PrintString("\nBytes of kernel : ");
	PrintHex(&bytesOfKernelELF, 4);

	uint64_t KernelVirtualMemSize = *((uint64_t*)(InfoTable + 0xc));	// Get size of kernel in virtual memory
	PrintString("\nKernel virtual memory size : ");
	PrintHex(&KernelVirtualMemSize, 8);

	// Check if enough space is available to load kernel as well as the elf (i.e. length of region > (KernelVirtualMemSize + bytesOfKernelELF))
	// We will load parsed kernel code from 2MiB physical memory (size : KernelVirtualMemSize)
	// Kernel ELF will be loaded at 2MiB + KernelVirtualMemSize + 4KiB physical memory form where it will be parsed
	bool enoughSpace = false;
	size_t numberMMAPentries = GetNumberOfMMAPEntries();
	struct ACPI3Entry* mmap = GetMMAPBase();
	for(size_t i=0; i<numberMMAPentries; i++)
	{
		if((mmap[i].BaseAddress <= (uint64_t)0x100000) && (mmap[i].Length > ((uint64_t)0x201000 - mmap[i].BaseAddress + (uint64_t)bytesOfKernelELF + KernelVirtualMemSize)))
		{
			enoughSpace = true;
			break;
		}
	}
	if(!enoughSpace)
	{
		PrintString("\nFatal Error : System Memory is fragmented too much.\nNot enough space to load kernel.\nCannot boot!");
		return 1;
	}
	PrintString("\nLoading kernel...");

	// We will be identity mapping the first 16 MiB of the physical memory
	// To see how the mapping is done refer to docs/mapping.txt
	SetupPAEPagingLongMode();

	// In GDT change base address of the 16-bit segments
	Setup16BitSegments(InfoTableAddress, LoadModuleAddress);

	/* ------
	We have 64 KiB free in physical memory from 0x80000 to 0x90000. The sector in our OS ISO image is 2 KiB in size.
	So we can load the kernel ELF in batches of 32 sectors.
	------ */
	KernelELFBase = (uint64_t)0x201000 + KernelVirtualMemSize;
	DAPKernel64Address->offset = 0x0;
	DAPKernel64Address->segment = 0x8000;
	DAPKernel64Address->NumberOfSectors = 32;
	for(uint16_t i=0; i<(KernelNumberOfSectors/32);i++)
	{
		LoadKernelELFSectors();
		memcopy(0x80000, KernelELFBase + i*0x10000,0x10000);
		DAPKernel64Address->FirstSector += 32;
	}

	PrintString("\nKernel loaded.");

	return 0;
}