#pragma once

#include <cstdint>

namespace Drivers {
	namespace PS2 {
		namespace Controller {
			enum Command : uint8_t {
				ReadConfiguration = 0x20,
				WriteConfiguration = 0x60,
				DisableMouse = 0xa7,
				EnableMouse = 0xa8,
				DisableKeyboard = 0xad,
				EnableKeyboard = 0xae
			};

			enum Configruation : uint8_t {
				KeyboardInterruptEnabled = 1,
				MouseInterruptEnabled = 2,
				KeyboardDisabled = 16,
				MouseDisabled = 32,
				IsScanCodeTranslated = 64
			};

			enum Status : uint8_t {
				OutputFull = 1,
				InputFull = 2,
				IsCommand = 8,
				TimeoutError = 64,
				ParityError = 128
			};

			extern uint16_t commandPort;
			extern uint16_t dataPort;

			extern bool initialize();
		}
	}
}
