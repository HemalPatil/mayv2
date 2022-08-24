#include "kernel.h"
#include "pml4t.h"

const size_t virPageSize = 0x1000;

const char* const initVirMemStr = "Initializing virtual memory management...\n";
const char* const initVirMemCompleteStr = "    Virtual memory management initialized\n";
const char* const pml4Marked = "    PML4 pages marked used in physical memory\n";

// Initializes virtual memory space for use by higher level dynamic memory manager and other kernel services
bool initializeVirtualMemory() {
	// TODO : add initialize vir mem implementation
	terminalPrintString(initVirMemStr, strlen(initVirMemStr));

	// TODO: mark the PML4 entry pages as used in phy mem
	// Crawl the 512 entries in PML4 table and mark pages as used in phy mem
	PML4E *pml4t = (PML4E*) infoTable->pml4eRoot;
	markPhysicalPagesAsUsed(pml4t, 1);
	// terminalPrintHex(&pml4t, sizeof(pml4t));
	// terminalPrintChar('\n');
	for (size_t i = 0; i < phyPageSize / sizeof(PML4E); ++i) {
		if (pml4t[i].present) {
			PDPTE *pdpt = pml4t[i].pageAddress << 12;
			markPhysicalPagesAsUsed(pdpt, 1);
			// terminalPrintString("    ", 4);
			// terminalPrintHex(&pdpt, sizeof(pdpt));
			// terminalPrintChar('\n');
			for (size_t j = 0; j < phyPageSize / sizeof(PDPTE); ++j) {
				if (pdpt[j].present) {
					PDE *pdt = pdpt[j].pageAddress << 12;
					markPhysicalPagesAsUsed(pdt, 1);
					// terminalPrintString("        ", 8);
					// terminalPrintHex(&pdt, sizeof(pdt));
					// terminalPrintChar('\n');
					for (size_t k = 0; k < phyPageSize / sizeof(PDE); ++k) {
						if (pdt[k].present) {
							PTE *pt = pdt[k].pageAddress << 12;
							markPhysicalPagesAsUsed(pt, 1);
							// terminalPrintString("            ", 12);
							// terminalPrintHex(&pt, sizeof(pt));
							// terminalPrintChar('\n');
							for (size_t l = 0; l < phyPageSize / sizeof(PTE); ++l) {
								if (pt[l].present) {
									void* pageAddress = pt[l].pageAddress << 12;
									// markPhysicalPagesAsUsed(pageAddress, 1);
									// terminalPrintString("                ", 16);
									// terminalPrintHex(&pageAddress, sizeof(pageAddress));
									// terminalPrintChar('\n');
								}
							}
						}
					}
				}
			}
		}
	}

	return true;

	terminalPrintString(initVirMemCompleteStr, strlen(initVirMemCompleteStr));
	return true;
}