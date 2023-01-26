#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/ps2/keyboard.h>
#include <idt64.h>
#include <kernel.h>
#include <terminal.h>

static const char* const keyStr = "key ";
static const char* const initIntrStr = "Initializing PS2 keyboard";

bool initializePs2Keyboard() {
	terminalPrintString(initIntrStr, strlen(initIntrStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	APIC::IORedirectionEntry keyEntry = APIC::readIoRedirectionEntry(Kernel::IRQ::Keyboard);
	installIdt64Entry(availableInterrupt, &ps2KeyboardHandlerWrapper);
	keyEntry.vector = availableInterrupt;
	keyEntry.deliveryMode = 0;
	keyEntry.destinationMode = 0;
	keyEntry.pinPolarity = 0;
	keyEntry.triggerMode = 0;
	keyEntry.mask = 0;
	keyEntry.destination = APIC::bootCpu;
	writeIoRedirectionEntry(Kernel::IRQ::Keyboard, keyEntry);
	++availableInterrupt;

	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	terminalPrintChar('\n');
	return true;
}

void ps2KeyboardHandler() {
	terminalPrintString(keyStr, strlen(keyStr));
	APIC::acknowledgeLocalInterrupt();
}
