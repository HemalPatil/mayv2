#include <apic.h>
#include <commonstrings.h>
#include <idt64.h>
#include <interrupts.h>
#include <kernel.h>
#include <string.h>
#include <terminal.h>

static const char* const initIntrStr = "Initializing interrupts";
static const char* const intrInitCompleteStr = "Interrupts initialized\n\n";
static const char* const enablingKeyStr = "Enabling keyboard interrupts";

bool initializeInterrupts() {
	terminalPrintString(initIntrStr, strlen(initIntrStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Enable keyboard interrupt
	// ISR for keyboard interrupts is at offset 0x20 in the IDT64
	terminalPrintSpaces4();
	terminalPrintString(enablingKeyStr, strlen(enablingKeyStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	IOAPICRedirectionEntry keyEntry = readIoRedirectionEntry(IRQ_KEYBOARD);
	keyEntry.vector = 0x20;
	keyEntry.deliveryMode = 0;
	keyEntry.destinationMode = 0;
	keyEntry.pinPolarity = 0;
	keyEntry.triggerMode = 0;
	keyEntry.mask = 0;
	// FIXME: assumes CPU 0 is the boot and current CPU
	keyEntry.destination = 0;
	writeIoRedirectionEntry(IRQ_KEYBOARD, keyEntry);
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');

	terminalPrintSpaces4();
	enableInterrupts();

	terminalPrintString(intrInitCompleteStr, strlen(intrInitCompleteStr));
	return true;
}
