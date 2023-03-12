#include <commonstrings.h>
#include <cstring>
#include <drivers/ps2/controller.h>
#include <drivers/timers.h>
#include <io.h>
#include <kernel.h>
#include <terminal.h>

static const char* const initStr = "Initializing PS2 controller";
static const char* const noKeyStr = "keyboard missing";
static const char* const noMouseStr = "mouse missing";

uint16_t Drivers::PS2::Controller::commandPort = 0x64;
uint16_t Drivers::PS2::Controller::dataPort = 0x60;

bool Drivers::PS2::Controller::initialize() {
	// FIXME: assumes a PS/2 controller is present

	terminalPrintString(initStr, strlen(initStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	Kernel::IDT::disableInterrupts();

	// Disable both keyboard and mouse to determine if both ports are supported
	IO::outputByte(commandPort, Command::DisableKeyboard);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(commandPort, Command::DisableMouse);
	Drivers::Timers::spinDelay(10000);

	// Flush the data buffer
	IO::inputByte(dataPort);
	Drivers::Timers::spinDelay(10000);

	// Make sure both ports are supported by reading the configuration byte
	IO::outputByte(commandPort, Command::ReadConfiguration);
	if (!isBufferFull(true, true)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	const uint8_t configuration = IO::inputByte(dataPort);
	if (!(configuration & Configruation::KeyboardDisabled)) {
		terminalPrintString(noKeyStr, strlen(noKeyStr));
		terminalPrintChar('\n');
		return false;
	}
	if (!(configuration & Configruation::MouseDisabled)) {
		terminalPrintString(noMouseStr, strlen(noMouseStr));
		terminalPrintChar('\n');
		return false;
	}

	// Enable both keyboard and mouse, and disable keyboard scan code translation
	const uint8_t newConfiguration = 4;
	IO::outputByte(commandPort, Command::WriteConfiguration);
	if (isBufferFull(false, true)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	IO::outputByte(dataPort, newConfiguration);
	Drivers::Timers::spinDelay(10000);

	// Make sure the new configuration was actually applied
	IO::outputByte(commandPort, Command::ReadConfiguration);
	Drivers::Timers::spinDelay(10000);
	if (!isBufferFull(true, true)) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}
	if (IO::inputByte(dataPort) != newConfiguration) {
		terminalPrintString(failedStr, strlen(failedStr));
		terminalPrintChar('\n');
		return false;
	}

	Kernel::IDT::enableInterrupts();
	terminalPrintString(doneStr, strlen(doneStr));
	terminalPrintChar('\n');
	return true;
}

bool Drivers::PS2::Controller::isCommandAcknowledged() {
	Timers::spinDelay(10000);
	const auto byte = IO::inputByte(dataPort);
	return byte == 0xfa;
}

bool Drivers::PS2::Controller::isBufferFull(bool isOutput, bool fromController) {
	Timers::spinDelay(10000);
	const auto status = IO::inputByte(commandPort);
	if (
		!(status & (isOutput ? Status::OutputFull : Status::InputFull)) ||
		(fromController && !(status & Status::FromController)) ||
		(!fromController && (status & Status::FromController)) ||
		(status & Status::TimeoutError) ||
		(status & Status::ParityError)
	) {
		return false;
	}
	return true;
}
