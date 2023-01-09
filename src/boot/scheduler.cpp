#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/timers/hpet.h>
#include <idt64.h>
#include <kernel.h>
#include <terminal.h>

static const char* const initSchedulerStr = "Initializing scheduler";
static const char* const initSchedulerCompleteStr = "Scheduler initialized\n";
static const char* const checkHpetStr = "Checking HPET presence";
static const char* const timerInitFailedStr = "Failed to initialize periodic timers\n";
static const char* const schedStr = "schd";
static const char* const schedulerStartStr = "Scheduler started";

static void enableHpet();

Kernel::Scheduler::TimerType Kernel::Scheduler::timerUsed = Kernel::Scheduler::TimerType::None;

void Kernel::Scheduler::start() {
	switch (timerUsed) {
		case HPET:
			enableHpet();
			break;
		default:
			terminalPrintString(timerInitFailedStr, strlen(timerInitFailedStr));
			panic();
	}
	terminalPrintString(schedulerStartStr, strlen(schedulerStartStr));
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
		if (hpetFound && Drivers::Timers::HPET::initialize()) {
			timerUsed = HPET;
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

static void enableHpet() {
	using namespace Drivers::Timers::HPET;

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
	APIC::writeIoRedirectionEntry(Kernel::IRQ::Timer, timerEntry);
	++availableInterrupt;
	// HPET registers must be written at 8-byte boundaries hence setting the bit fields directly is not possible
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
	uint64_t *configure = (uint64_t*)(void*)periodicTimer;
	*configure |= ((Kernel::IRQ::Timer << 9) | TimerParamters::Enable | TimerParamters::Periodic | TimerParamters::PeriodicInterval);
	// FIXME: change timer interval to some smaller value (possibly configurable), currently 1sec
	periodicTimer->comparatorValue = 1000000000000000UL / registers->period;
	configure = (uint64_t*)(void*)registers;
	configure[2] |= 1;
	#pragma GCC diagnostic pop
	timerInterruptCallback = Kernel::Scheduler::periodicTimerHandler;
}
