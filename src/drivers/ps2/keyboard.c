#include <apic.h>
#include <commonstrings.h>
#include <drivers/ps2/keyboard.h>
#include <idt64.h>
#include <string.h>
#include <terminal.h>

static const char* const keyStr = "key";
static const char* const initIntrStr = "Initializing PS2 keyboard";

bool initializePs2Keyboard() {
	terminalPrintString(initIntrStr, strlen(initIntrStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	IOAPICRedirectionEntry keyEntry = readIoRedirectionEntry(IRQ_KEYBOARD);
	installIdt64Entry(availableInterrupt, &keyboardHandlerWrapper);
	keyEntry.vector = availableInterrupt;
	keyEntry.deliveryMode = 0;
	keyEntry.destinationMode = 0;
	keyEntry.pinPolarity = 0;
	keyEntry.triggerMode = 0;
	keyEntry.mask = 0;
	keyEntry.destination = bootCpu;
	writeIoRedirectionEntry(IRQ_KEYBOARD, keyEntry);
	++availableInterrupt;

	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	return true;
}

void keyboardHandler() {
	terminalPrintString(keyStr, strlen(keyStr));
	acknowledgeLocalApicInterrupt();
}
