#pragma once

namespace Drivers {
	namespace PS2 {
		namespace Keyboard {
			extern bool initialize(uint32_t apicId);
		}
	}
}

// keyboard.cpp
extern "C" void ps2KeyboardHandler();

// keyboardasm.asm
extern "C" void ps2KeyboardHandlerWrapper();
