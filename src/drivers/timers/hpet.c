#include <acpi.h>
#include <commonstrings.h>
#include <drivers/timers/hpet.h>
#include <kernel.h>
// #include <phymemmgmt.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const char* const initStr = "Initializing HPET";
static const char* const initCompleteStr = "HPET initialized\n\n";
static const char* const mappingStr = "Mapping HPET registers to kernel address space";
static const char* const check64Str = "Checking clock period and 64-bit capability";

HPETRegisters *hpet = NULL;

bool initializeHpet() {
	terminalPrintString(initStr, strlen(initStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// TODO: complete HPET initialization
	// FIXME: should do HPET table checlsum verificaion
	terminalPrintSpaces4();
	terminalPrintString(mappingStr, strlen(mappingStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	ACPIHPETTable *hpetTable = (ACPIHPETTable*)hpetSdtHeader;
	PageRequestResult requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!mapVirtualPages(requestResult.address, (void*)hpetTable->address, 1, MEMORY_REQUEST_CACHE_DISABLE)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	hpet = (HPETRegisters*)requestResult.address;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(check64Str, strlen(check64Str));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	// HPET period should be within 0 and 100 nanseconds
	if (!hpet->bit64Capable || hpet->period == 0 || hpet->period > 100000000) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintDecimal(hpetTable->comparatorCount);
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintDecimal(hpet->timerCount);
	terminalPrintHex(&hpet->period, sizeof(hpet->period));
	terminalPrintChar('\n');

	sizeof(HPETRegisters);
	sizeof(HPETTimer);

	terminalPrintString(initCompleteStr, strlen(initCompleteStr));
	return true;
}
