#pragma once

#include <string>

namespace Unicode {
	extern uint32_t toCodePoint(const std::string &str);
	extern uint32_t toCodePoint(const std::u16string &str);
	extern uint32_t toCodePoint(const char16_t word1, const char16_t word2 = 0);
	extern std::string toUtf8(uint32_t codePoint);
	extern std::string toUtf8(const std::u16string &str);
	extern bool isValidUtf8(const std::string &str);
}
