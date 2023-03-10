#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/ps2/keyboard.h>
#include <kernel.h>
#include <terminal.h>

static const char* const keyStr = "key ";
static const char* const initIntrStr = "Initializing PS2 keyboard on CPU [";

bool Drivers::PS2::Keyboard::initialize(uint32_t apicId) {
	terminalPrintString(initIntrStr, strlen(initIntrStr));
	terminalPrintDecimal(apicId);
	terminalPrintChar(']');
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	Kernel::IDT::disableInterrupts();
	APIC::IORedirectionEntry keyEntry = APIC::readIoRedirectionEntry(Kernel::IRQ::Keyboard);
	Kernel::IDT::installEntry(Kernel::IDT::availableInterrupt, &ps2KeyboardHandlerWrapper, 2);
	keyEntry.vector = Kernel::IDT::availableInterrupt;
	keyEntry.deliveryMode = 0;
	keyEntry.destinationMode = 0;
	keyEntry.pinPolarity = 0;
	keyEntry.triggerMode = 0;
	keyEntry.mask = 0;
	keyEntry.destination = apicId;
	writeIoRedirectionEntry(Kernel::IRQ::Keyboard, keyEntry);
	++Kernel::IDT::availableInterrupt;
	Kernel::IDT::enableInterrupts();

	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	terminalPrintChar('\n');
	return true;
}

void ps2KeyboardHandler() {
	const uint64_t cpuApicId = Kernel::readMsr(Kernel::MSR::x2ApicId);
	terminalPrintChar('[');
	terminalPrintDecimal(cpuApicId);
	terminalPrintString("] ", 2);
	APIC::acknowledgeLocalInterrupt();
}
