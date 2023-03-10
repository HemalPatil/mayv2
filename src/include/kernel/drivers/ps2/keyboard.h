#pragma once

#include <cstdint>

namespace Drivers {
	namespace PS2 {
		namespace Keyboard {
			enum ScanCodeSet1 : uint64_t {
				None = 0,
				Esc
			};

			extern uint16_t inPort;

			extern bool initialize(uint32_t apicId);
		}
	}
}

// keyboard.cpp
extern "C" void ps2KeyboardHandler();

// keyboardasm.asm
extern "C" void ps2KeyboardHandlerWrapper();
