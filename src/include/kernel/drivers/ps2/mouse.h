#pragma once

#include <cstdint>

namespace Drivers {
	namespace PS2 {
		namespace Mouse {
			enum Command : uint8_t {
				EnableScanning = 0xf4
			};

			extern bool initialize(uint32_t apicId);
		}
	}
}

// mouse.cpp
extern "C" void ps2MouseHandler();

// mouseasm.asm
extern "C" void ps2MouseHandlerWrapper();
