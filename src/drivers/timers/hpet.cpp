#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/timers/hpet.h>
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

Drivers::Timers::HPET::Registers *Drivers::Timers::HPET::registers = nullptr;
Drivers::Timers::HPET::Timer *Drivers::Timers::HPET::periodicTimer = nullptr;
void (*Drivers::Timers::HPET::timerInterruptCallback)() = nullptr;

bool Drivers::Timers::HPET::initialize() {
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
	registers = (Registers*)requestResult.address;
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	// HPET should be 64-bit capable and its period should be within 0 and 100 nanseconds
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(check64Str, strlen(check64Str));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!registers->bit64Capable || registers->period == 0 || registers->period > 100000000) {
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
	terminalPrintDecimal(registers->timerCount + 1);
	terminalPrintChar('\n');

	// Find at least one timer with 64-bit and periodic interrupt capability
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(periodicTimerStr, strlen(periodicTimerStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	for (size_t i = 0; i < (size_t)registers->timerCount + 1; ++i) {
		if (
			registers->timers[i].bit64Capable &&
			!registers->timers[i].interruptType &&
			registers->timers[i].periodicCapable
		) {
			periodicTimer = &registers->timers[i];
			break;
		}
	}
	if (!periodicTimer) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(presentStr, strlen(presentStr));
		terminalPrintChar('\n');
		return false;
	}
	terminalPrintString(presentStr, strlen(presentStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	terminalPrintString(initHpetCompleteStr, strlen(initHpetCompleteStr));
	return true;
}

void hpetHandler() {
	APIC::acknowledgeLocalInterrupt();
	if (Drivers::Timers::HPET::timerInterruptCallback) {
		Drivers::Timers::HPET::timerInterruptCallback();
	} else {
		terminalPrintString(hpetStr, strlen(hpetStr));
	}
}
