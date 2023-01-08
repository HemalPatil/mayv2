#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/timers/hpet.h>
#include <idt64.h>
#include <kernel.h>
#include <terminal.h>

static const char* const initHpetStr = "Initializing HPET";
static const char* const initHpetCompleteStr = "HPET initialized\n";
static const char* const mappingStr = "Mapping HPET registers to kernel address space";
static const char* const check64Str = "Checking clock period and 64-bit capability";
static const char* const periodicTimerStr = "Checking for 64-bit capable edge-triggered periodic timer";
static const char* const minTickStr = "Minimum tick ";
static const char* const timerCountStr = ", Timers - ";
static const char* const hpetStr = "hpet";

Drivers::Timers::HPET::Registers *Drivers::Timers::HPET::hpet = nullptr;
Drivers::Timers::HPET::Timer *Drivers::Timers::HPET::hpetPeriodicTimer = nullptr;

static void (*timerInterruptCallback)() = nullptr;

bool Drivers::Timers::HPET::initialize(size_t intervalNanoseconds, void (*timerCallback)()) {
	terminalPrintSpaces4();
	terminalPrintString(initHpetStr, strlen(initHpetStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// FIXME: should do HPET table checksum verificaion
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(mappingStr, strlen(mappingStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	Info *hpetTable = (Info*)ACPI::hpetSdtHeader;
	Kernel::Memory::PageRequestResult requestResult = Kernel::Memory::Virtual::requestPages(
		1,
		Kernel::Memory::RequestType::Kernel | Kernel::Memory::RequestType::Contiguous
	);
	if (
		requestResult.address == INVALID_ADDRESS ||
		requestResult.allocatedCount != 1 ||
		!Kernel::Memory::Virtual::mapPages(
			requestResult.address,
			(void*)hpetTable->address,
			1,
			Kernel::Memory::RequestType::CacheDisable
		)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	hpet = (Registers*)requestResult.address;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// HPET should be 64-bit capable and its period should be within 0 and 100 nanseconds
	terminalPrintSpaces4();
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
	terminalPrintSpaces4();
	terminalPrintString(minTickStr, strlen(minTickStr));
	terminalPrintHex(&hpetTable->minimumTick, sizeof(hpetTable->minimumTick));
	terminalPrintString(timerCountStr, strlen(timerCountStr));
	terminalPrintDecimal(hpet->timerCount + 1);
	terminalPrintChar('\n');

	// Find at least one timer with 64-bit and periodic interrupt capability
	terminalPrintSpaces4();
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
			break;
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

	// Install the timer IRQ handler
	APIC::IORedirectionEntry timerEntry = APIC::readIoRedirectionEntry(Kernel::IRQ::Timer);
	installIdt64Entry(availableInterrupt, &hpetHandlerWrapper);
	timerEntry.vector = availableInterrupt;
	timerEntry.deliveryMode = 0;
	timerEntry.destinationMode = 0;
	timerEntry.pinPolarity = 0;
	timerEntry.triggerMode = 0;
	timerEntry.mask = 0;
	timerEntry.destination = APIC::bootCpu;
	writeIoRedirectionEntry(Kernel::IRQ::Timer, timerEntry);
	++availableInterrupt;
	// HPET registers must be written at 8-byte boundaries hence setting the bit fields directly is not possible
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
	uint64_t *configure = (uint64_t*)(void*)hpetPeriodicTimer;
	*configure |= ((Kernel::IRQ::Timer << 9) | TimerParamters::Enable | TimerParamters::Periodic | TimerParamters::PeriodicInterval);
	hpetPeriodicTimer->comparatorValue = 1000000UL * intervalNanoseconds / hpet->period;
	configure = (uint64_t*)(void*)hpet;
	configure[2] |= 1;
	#pragma GCC diagnostic pop

	timerInterruptCallback = timerCallback;

	terminalPrintSpaces4();
	terminalPrintString(initHpetCompleteStr, strlen(initHpetCompleteStr));
	return true;
}

void hpetHandler() {
	APIC::acknowledgeLocalInterrupt();
	if (timerInterruptCallback) {
		timerInterruptCallback();
	} else {
		terminalPrintString(hpetStr, strlen(hpetStr));
	}
}
