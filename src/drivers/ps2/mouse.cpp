#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/ps2/controller.h>
#include <drivers/ps2/mouse.h>
#include <drivers/timers.h>
#include <io.h>
#include <terminal.h>

static const char* const initStr = "Initializing PS2 mouse on CPU [";
static const char* const namespaceStr = "Drivers::PS2::Mouse::";
static const char* const enableScanFailStr = "initialize failed to enable scanning";

bool Drivers::PS2::Mouse::initialize(uint32_t apicId) {
	// Assumes the PS/2 controller has been properly initialized
	// using Drivers::PS2::Controller::initialize()

	terminalPrintString(initStr, strlen(initStr));
	terminalPrintDecimal(apicId);
	terminalPrintChar(']');
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	// Enable scanning
	IO::outputByte(Controller::commandPort, Controller::Command::WriteToMouse);
	if (Controller::isBufferFull(false, false)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	IO::outputByte(Controller::dataPort, Mouse::Command::EnableScanning);
	if (!Controller::isCommandAcknowledged()) {
		terminalPrintString(namespaceStr, strlen(namespaceStr));
		terminalPrintString(enableScanFailStr, strlen(enableScanFailStr));
		return false;
	}

	// Enable the mouse interrupt in PS2 controller configuration
	Kernel::IDT::disableInterrupts();
	IO::outputByte(Controller::commandPort, Controller::Command::ReadConfiguration);
	if (!Controller::isBufferFull(true, true)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	const uint8_t configuration = IO::inputByte(Controller::dataPort);
	const uint8_t newConfiguration =
		Controller::Configruation::MouseInterruptEnabled |
		(configuration & ~Controller::Configruation::MouseDisabled);
	IO::outputByte(Controller::commandPort, Controller::Command::WriteConfiguration);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(Controller::dataPort, newConfiguration);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(Controller::commandPort, Controller::Command::ReadConfiguration);
	Drivers::Timers::spinDelay(10000);
	if (IO::inputByte(Controller::dataPort) != newConfiguration) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}

	// Install the IRQ handler
	APIC::IORedirectionEntry mouseEntry = APIC::readIoRedirectionEntry(Kernel::IRQ::Mouse);
	Kernel::IDT::installEntry(Kernel::IDT::availableInterrupt, &ps2MouseHandlerWrapper, 2);
	mouseEntry.vector = Kernel::IDT::availableInterrupt;
	mouseEntry.deliveryMode = 0;
	mouseEntry.destinationMode = 0;
	mouseEntry.pinPolarity = 0;
	mouseEntry.triggerMode = 0;
	mouseEntry.mask = 0;
	mouseEntry.destination = apicId;
	writeIoRedirectionEntry(Kernel::IRQ::Mouse, mouseEntry);
	++Kernel::IDT::availableInterrupt;
	Kernel::IDT::enableInterrupts();

	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	return true;
}

size_t i = 0;

void ps2MouseHandler() {
	using namespace Drivers::PS2::Mouse;

	const auto byte = IO::inputByte(Drivers::PS2::Controller::dataPort);
	terminalPrintHex(&byte, 1);
	++i;
	if (i == 3) {
		i = 0;
		terminalPrintChar('\n');
	}
	APIC::acknowledgeLocalInterrupt();
}
