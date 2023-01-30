#include <acpi.h>
#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/timers/hpet.h>
#include <idt64.h>
#include <kernel.h>
#include <queue>
#include <terminal.h>

#define SCHEDULER_EVENT_DISPATCH_LIMIT 100

static const char* const initSchedulerStr = "Initializing scheduler";
static const char* const initSchedulerCompleteStr = "Scheduler initialized\n\n";
static const char* const checkHpetStr = "Checking HPET presence";
static const char* const timerInitFailedStr = "Failed to initialize periodic timers\n";

static std::queue<std::coroutine_handle<>> eventQueue;

static void enableHpet();

Kernel::Scheduler::TimerType Kernel::Scheduler::timerUsed = Kernel::Scheduler::TimerType::None;

void Kernel::Scheduler::timerLoop() {
	// Dispatch events synchronously until the queue has dispatchable events and < SCHEDULER_EVENT_DISPATCH_LIMIT in 1 loop
	size_t dispatchedEventsCount = 0;
	while (!eventQueue.empty() && dispatchedEventsCount < SCHEDULER_EVENT_DISPATCH_LIMIT) {
		std::coroutine_handle<> x = eventQueue.front();
		if (x && !x.done()) {
			x.resume();
		}
		eventQueue.pop();
		++dispatchedEventsCount;
	}

	// TODO: do rest of scheduling
}

void Kernel::Scheduler::queueEvent(std::coroutine_handle<> event) {
	eventQueue.push(event);
}

bool Kernel::Scheduler::start() {
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

	// Initialization of any timer drivers implemented in the future will go here

	if (!timerInitialized) {
		terminalPrintString(timerInitFailedStr, strlen(timerInitFailedStr));
		return false;
	}

	switch (timerUsed) {
		case HPET:
			enableHpet();
			break;
		default:
			terminalPrintString(timerInitFailedStr, strlen(timerInitFailedStr));
			panic();
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
	timerEntry.destination = APIC::bootCpuId;
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
	timerInterruptCallback = Kernel::Scheduler::timerLoop;
}
