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
	// with scan code translation turned off
	// using Drivers::PS2::Controller::initialize()

	terminalPrintString(initKeyStr, strlen(initKeyStr));
	terminalPrintDecimal(apicId);
	terminalPrintChar(']');
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));

	// Set scan code set 2
	IO::outputByte(Controller::dataPort, 0xf0);
	Timers::spinDelay(10000);
	IO::outputByte(Controller::dataPort, 2);

	// Enable the keyboard interrupt in PS2 controller configuration
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

uint64_t Drivers::PS2::Keyboard::validScanCodes[] = {
	Pressed_F9, Pressed_F5, Pressed_F3, Pressed_F1, Pressed_F2,
	Pressed_F12, Pressed_F10, Pressed_F8, Pressed_F6, Pressed_F4,
	Pressed_Tab, Pressed_BackTick, Pressed_AltLeft, Pressed_ShiftLeft, Pressed_CtrlLeft,
	Pressed_Q, Pressed_Num1, Pressed_Z, Pressed_S, Pressed_A,
	Pressed_W, Pressed_Num2, Pressed_C, Pressed_X, Pressed_D,
	Pressed_E, Pressed_Num4, Pressed_Num3, Pressed_Space, Pressed_V,
	Pressed_F, Pressed_T, Pressed_R, Pressed_Num5, Pressed_N,
	Pressed_B, Pressed_H, Pressed_G, Pressed_Y, Pressed_Num6,
	Pressed_M, Pressed_J, Pressed_U, Pressed_Num7, Pressed_Num8,
	Pressed_Comma, Pressed_K, Pressed_I, Pressed_O, Pressed_Num0,
	Pressed_Num9, Pressed_Dot, Pressed_Slash, Pressed_L, Pressed_SemiColon,
	Pressed_P, Pressed_Minus, Pressed_Quote, Pressed_BracketOpen, Pressed_Equals,
	Pressed_CapsLock, Pressed_ShiftRight, Pressed_Enter, Pressed_BracketClose, Pressed_BackSlash,
	Pressed_Backspace, Pressed_Keypad1, Pressed_Keypad4, Pressed_Keypad7, Pressed_Keypad0,
	Pressed_KeypadDot, Pressed_Keypad2, Pressed_Keypad5, Pressed_Keypad6, Pressed_Keypad8,
	Pressed_Escape, Pressed_NumLock, Pressed_F11, Pressed_KeypadPlus, Pressed_Keypad3,
	Pressed_KeypadMinus, Pressed_KeypadMultiply, Pressed_Keypad9, Pressed_ScrollLock, Pressed_F7,
	Pressed_AltRight, Pressed_CtrlRight, Pressed_MediaTrackPrevious, Pressed_WinLeft, Pressed_MediaVolumeDown,
	Pressed_MediaMute, Pressed_WinRight, Pressed_MediaCalculator, Pressed_MediaVolumeUp, Pressed_MediaPlayPause,
	Pressed_AcpiPower, Pressed_MediaStop, Pressed_AcpiSleep, Pressed_KeypadSlash, Pressed_MediaTrackNext,
	Pressed_KeypadEnter, Pressed_AcpiWake, Pressed_End, Pressed_Left, Pressed_Home,
	Pressed_Insert, Pressed_Delete, Pressed_Down, Pressed_Right, Pressed_Up,
	Pressed_PageDown, Pressed_PageUp, Pressed_PrintScreen, Pressed_PauseBreak,

	Released_F9, Released_F5, Released_F3, Released_F1, Released_F2,
	Released_F12, Released_F10, Released_F8, Released_F6, Released_F4,
	Released_Tab, Released_BackTick, Released_AltLeft, Released_ShiftLeft, Released_CtrlLeft,
	Released_Q, Released_Num1, Released_Z, Released_S, Released_A,
	Released_W, Released_Num2, Released_C, Released_X, Released_D,
	Released_E, Released_Num4, Released_Num3, Released_Space, Released_V,
	Released_F, Released_T, Released_R, Released_Num5, Released_N,
	Released_B, Released_H, Released_G, Released_Y, Released_Num6,
	Released_M, Released_J, Released_U, Released_Num7, Released_Num8,
	Released_Comma, Released_K, Released_I, Released_O, Released_Num0,
	Released_Num9, Released_Dot, Released_Slash, Released_L, Released_SemiColon,
	Released_P, Released_Minus, Released_Quote, Released_BracketOpen, Released_Equals,
	Released_CapsLock, Released_ShiftRight, Released_Enter, Released_BracketClose, Released_BackSlash,
	Released_Backspace, Released_Keypad1, Released_Keypad4, Released_Keypad7, Released_Keypad0,
	Released_KeypadDot, Released_Keypad2, Released_Keypad5, Released_Keypad6, Released_Keypad8,
	Released_Escape, Released_NumLock, Released_F11, Released_KeypadPlus, Released_Keypad3,
	Released_KeypadMinus, Released_KeypadMultiply, Released_Keypad9, Released_ScrollLock, Released_F7,
	Released_AltRight, Released_CtrlRight, Released_MediaTrackPrevious, Released_WinLeft, Released_MediaVolumeDown,
	Released_MediaMute, Released_WinRight, Released_MediaCalculator, Released_MediaVolumeUp, Released_MediaPlayPause,
	Released_AcpiPower, Released_MediaStop, Released_AcpiSleep, Released_KeypadSlash, Released_MediaTrackNext,
	Released_KeypadEnter, Released_AcpiWake, Released_End, Released_Left, Released_Home,
	Released_Insert, Released_Delete, Released_Down, Released_Right, Released_Up,
	Released_PageDown, Released_PageUp, Released_PrintScreen
};
