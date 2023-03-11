#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/ps2/controller.h>
#include <drivers/ps2/keyboard.h>
#include <drivers/timers.h>
#include <io.h>
#include <kernel.h>
#include <terminal.h>

static const char* const keyStr = "key ";
static const char* const initKeyStr = "Initializing PS2 keyboard on CPU [";

static uint64_t scanCodeBuffer = 0;

bool Drivers::PS2::Keyboard::initialize(uint32_t apicId) {
	// Assumes the PS/2 controller has been properly initialized
	// using Drivers::PS2::Controller::initialize()

	terminalPrintString(initKeyStr, strlen(initKeyStr));
	terminalPrintDecimal(apicId);
	terminalPrintChar(']');
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	// Enable the keyboard interrupt in PS2 controller configuration
	// with scan code translation turned off
	Kernel::IDT::disableInterrupts();
	IO::outputByte(Controller::commandPort, Controller::Command::ReadConfiguration);
	Drivers::Timers::spinDelay(10000);
	const uint8_t status = IO::inputByte(Controller::commandPort);
	if (
		!(status & Controller::Status::OutputFull) ||
		!(status & Controller::Status::IsCommand) ||
		(status & Controller::Status::TimeoutError) ||
		(status & Controller::Status::ParityError)
	) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	const uint8_t configuration = IO::inputByte(Controller::dataPort);
	const uint8_t newConfiguration =
		Controller::Configruation::KeyboardInterruptEnabled |
		(configuration & ~Controller::Configruation::KeyboardDisabled) |
		(configuration & ~Controller::Configruation::IsScanCodeTranslated);
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

	// Set scan code set 2
	IO::outputByte(Controller::dataPort, 0xf0);
	Timers::spinDelay(10000);
	IO::outputByte(Controller::dataPort, 2);

	// Install the IRQ handler
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
	const auto byte = IO::inputByte(Drivers::PS2::Controller::dataPort);
	scanCodeBuffer <<= 8;
	scanCodeBuffer |= byte;
	terminalPrintHex(&byte, 1);
	APIC::acknowledgeLocalInterrupt();
}
