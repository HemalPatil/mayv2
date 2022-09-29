#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <drivers/timers/hpet.h>
#include <idt64.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const char* const initStr = "Initializing HPET";
static const char* const initCompleteStr = "HPET initialized\n\n";
static const char* const mappingStr = "Mapping HPET registers to kernel address space";
static const char* const check64Str = "Checking clock period and 64-bit capability";
static const char* const periodicTimerStr = "Checking for 64-bit capable edge-triggered periodic timer";
static const char* const minTickStr = "Minimum tick ";
static const char* const hpetStr = "hpet";

HPETRegisters *hpet = NULL;
HPETTimer *hpetPeriodicTimer = NULL;

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

	// HPET should be 64-bit capable and its period should be within 0 and 100 nanseconds
	terminalPrintSpaces4();
	terminalPrintString(check64Str, strlen(check64Str));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!hpet->bit64Capable || hpet->period == 0 || hpet->period > 100000000) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(minTickStr, strlen(minTickStr));
	terminalPrintHex(&hpetTable->minimumTick, sizeof(hpetTable->minimumTick));
	terminalPrintChar('\n');

	// Find at least one timer with 64-bit and periodic interrupt capability
	terminalPrintSpaces4();
	terminalPrintString(periodicTimerStr, strlen(periodicTimerStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	for (size_t i = 0; i < (size_t)hpet->timerCount + 1; ++i) {
		if (
			hpet->timers[i].bit64Capable &&
			!hpet->timers[i].interruptType &&
			hpet->timers[i].periodicCapable
		) {
			hpetPeriodicTimer = &hpet->timers[i];
		}
	}
	if (!hpetPeriodicTimer) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(presentStr, strlen(presentStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(presentStr, strlen(presentStr));
	terminalPrintChar('\n');

	IOAPICRedirectionEntry keyEntry = readIoRedirectionEntry(IRQ_HPET_PERIODIC_TIMER);
	terminalPrintHex(&availableInterrupt, 1);
	installIdt64Entry(availableInterrupt, &hpetHandlerWrapper);
	keyEntry.vector = availableInterrupt;
	keyEntry.deliveryMode = 0;
	keyEntry.destinationMode = 0;
	keyEntry.pinPolarity = 0;
	keyEntry.triggerMode = 0;
	keyEntry.mask = 0;
	keyEntry.destination = bootCpu;
	writeIoRedirectionEntry(IRQ_HPET_PERIODIC_TIMER, keyEntry);
	++availableInterrupt;
	// HPET registers must be written 8-byte boundaries hence setting the bit fields directly is not possible
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
	uint64_t *configure = (uint64_t*)(void*)hpetPeriodicTimer;
	*configure |= ((IRQ_HPET_PERIODIC_TIMER << 9) | HPET_TIMER_PERIODIC | HPET_TIMER_PERIODIC_INTERVAL);
	#pragma GCC diagnostic pop

	terminalPrintString(initCompleteStr, strlen(initCompleteStr));
	return true;
}

void hpetHandler() {
	terminalPrintString(hpetStr, strlen(hpetStr));
	acknowledgeLocalApicInterrupt();
}
