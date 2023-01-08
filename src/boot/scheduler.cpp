#include <acpi.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/timers/hpet.h>
#include <kernel.h>
#include <terminal.h>

static const char* const initSchedulerStr = "Initializing scheduler";
static const char* const initSchedulerCompleteStr = "Scheduler initialized\n";
static const char* const checkHpetStr = "Checking HPET presence";
static const char* const timerInitFailedStr = "Failed to initialize periodic timers\n";
static const char* const schedStr = "schd";

void Kernel::Scheduler::start() {
	while (true) {
		haltSystem();
	}
}

void Kernel::Scheduler::periodicTimerHandler() {
	terminalPrintString(schedStr, strlen(schedStr));
}

bool Kernel::Scheduler::initialize() {
	terminalPrintString(initSchedulerStr, strlen(initSchedulerStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Find a timer that can generate periodic interrupts
	// Currently only HPET drivers are present
	bool timerInitialized = false;

	// Check HPET presence and use it if present
	if (!timerInitialized) {
		terminalPrintSpaces4();
		terminalPrintString(checkHpetStr, strlen(checkHpetStr));
		terminalPrintString(ellipsisStr, strlen(ellipsisStr));
		bool hpetFound = false;
		if (ACPI::hpetSdtHeader != INVALID_ADDRESS && ACPI::hpetSdtHeader) {
			hpetFound = true;
		}
		if (!hpetFound) {
			terminalPrintString(notStr, strlen(notStr));
			terminalPrintChar(' ');
		}
		terminalPrintString(presentStr, strlen(presentStr));
		terminalPrintChar('\n');
		// FIXME: change timer interval to some smaller value (possibly configurable), currently 1sec
		if (hpetFound && Drivers::Timers::HPET::initialize(1000000000UL, periodicTimerHandler)) {
			timerInitialized = true;
		}
	}

	if (!timerInitialized) {
		terminalPrintString(timerInitFailedStr, strlen(timerInitFailedStr));
		return false;
	}

	terminalPrintString(initSchedulerCompleteStr, strlen(initSchedulerCompleteStr));
	return true;
}
