#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/ps2/keyboard.h>
#include <io.h>
#include <kernel.h>
#include <terminal.h>

static const char* const keyStr = "key ";
static const char* const initKeyStr = "Initializing PS2 keyboard on CPU [";

static uint64_t scanCodeBuffer = 0;

uint16_t Drivers::PS2::Keyboard::inPort = 0x60;

bool Drivers::PS2::Keyboard::initialize(uint32_t apicId) {
	// FIXME: Assumes the BIOS initialized the PS/2 controller and devices correctly
	// Should perhaps add a PS/2 controller initializer

	terminalPrintString(initKeyStr, strlen(initKeyStr));
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
	// FIXME: Assumes the PS/2 keyboard is using scan code set 1
	const auto byte = IO::inputByte(Drivers::PS2::Keyboard::inPort);
	scanCodeBuffer <<= 8;
	scanCodeBuffer |= byte;
	terminalPrintHex(&byte, 1);
	APIC::acknowledgeLocalInterrupt();
}
