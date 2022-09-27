#include <apic.h>
#include <commonstrings.h>
#include <drivers/ps2/keyboard.h>
#include <idt64.h>
#include <interrupts.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>

static const char* const initIntrStr = "Initializing interrupts";
static const char* const intrInitCompleteStr = "Interrupts initialized\n\n";
static const char* const enablingKeyStr = "Setting up keyboard interrupts";

size_t availableInterrupt = 0x20;

bool initializeInterrupts() {
	terminalPrintString(initIntrStr, strlen(initIntrStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Enable keyboard interrupt
	terminalPrintSpaces4();
	terminalPrintString(enablingKeyStr, strlen(enablingKeyStr));
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
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	++availableInterrupt;
	terminalPrintSpaces4();
	enableInterrupts();

	terminalPrintString(intrInitCompleteStr, strlen(intrInitCompleteStr));
	return true;
}

void acknowledgeLocalApicInterrupt() {
	getLocalApic()->endOfInterrupt = 0;
}
