#include <kernel.h>
#include <phymemmgmt.h>
#include <pml4t.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

const char* const initVirMemStr = "Initializing virtual memory management...\n";
const char* const initVirMemCompleteStr = "    Virtual memory management initialized\n";
const char* const pml4Marked = "    PML4 pages marked used in physical memory\n";

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool initializeVirtualMemory(void* kernelElfBase, uint64_t kernelElfSize, ELF64ProgramHeader* programHeader) {
	// TODO : add initialize vir mem implementation
	terminalPrintString(initVirMemStr, strlen(initVirMemStr));

	// loader32 has identity mapped first 16MiB making things easy till now
	// Create new PML4 table which maps only the necessary structures
	
	// TODO: Move important structures like physical memory bitmap to higher half

	// Free up kernel ELF in physical memory
	markPhysicalPages((void*)kernelElfBase, kernelElfSize / pageSize, PhyMemEntry_Free);

	// TODO: mark the PML4 entry pages as used in phy mem
	// Crawl the 512 entries in PML4 table and mark pages as used in phy mem
	// PML4E *pml4t = (PML4E*) infoTable->pml4eRoot;
	// markPhyPages(pml4t, 1);
	// // terminalPrintHex(&pml4t, sizeof(pml4t));
	// // terminalPrintChar('\n');
	// for (size_t i = 0; i < pageSize / sizeof(PML4E); ++i) {
	// 	if (pml4t[i].present) {
	// 		PDPTE *pdpt = pml4t[i].pageAddress << 12;
	// 		markPhyPages(pdpt, 1);
	// 		// terminalPrintString("    ", 4);
	// 		// terminalPrintHex(&pdpt, sizeof(pdpt));
	// 		// terminalPrintChar('\n');
	// 		for (size_t j = 0; j < pageSize / sizeof(PDPTE); ++j) {
	// 			if (pdpt[j].present) {
	// 				PDE *pdt = pdpt[j].pageAddress << 12;
	// 				markPhyPages(pdt, 1);
	// 				// terminalPrintString("        ", 8);
	// 				// terminalPrintHex(&pdt, sizeof(pdt));
	// 				// terminalPrintChar('\n');
	// 				for (size_t k = 0; k < pageSize / sizeof(PDE); ++k) {
	// 					if (pdt[k].present) {
	// 						PTE *pt = pdt[k].pageAddress << 12;
	// 						markPhyPages(pt, 1);
	// 						// terminalPrintString("            ", 12);
	// 						// terminalPrintHex(&pt, sizeof(pt));
	// 						// terminalPrintChar('\n');
	// 						for (size_t l = 0; l < pageSize / sizeof(PTE); ++l) {
	// 							if (pt[l].present) {
	// 								void* pageAddress = pt[l].pageAddress << 12;
	// 								// markPhyPages(pageAddress, 1);
	// 								// terminalPrintString("                ", 16);
	// 								// terminalPrintHex(&pageAddress, sizeof(pageAddress));
	// 								// terminalPrintChar('\n');
	// 							}
	// 						}
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	return true;

	terminalPrintString(initVirMemCompleteStr, strlen(initVirMemCompleteStr));
	return true;
}