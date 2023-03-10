#pragma once

#include <cstdint>

namespace IO {
	extern "C" uint8_t inputByte(uint16_t port);
	extern "C" void outputByte(uint16_t port, uint8_t byte);
}
