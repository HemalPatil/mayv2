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
	IO::outputByte(commandPort, Command::DisableKeyboard);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(commandPort, Command::DisableMouse);
	Drivers::Timers::spinDelay(10000);
	IO::inputByte(dataPort);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(commandPort, Command::ReadConfiguration);
	Drivers::Timers::spinDelay(10000);
	const uint8_t status = IO::inputByte(commandPort);
	if (
		!(status & Status::OutputFull) ||
		!(status & Status::IsCommand) ||
		(status & Status::TimeoutError) ||
		(status & Status::ParityError)
	) {
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
	const uint8_t newConfiguration = 4;
	IO::outputByte(commandPort, Command::WriteConfiguration);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(dataPort, newConfiguration);
	Drivers::Timers::spinDelay(10000);
	IO::outputByte(commandPort, Command::ReadConfiguration);
	Drivers::Timers::spinDelay(10000);
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
