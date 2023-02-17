#include <cstring>
#include <kernel.h>
#include <terminal.h>
#include <unicode.h>

static const char* const invalidStr = "Invalid UTF-";
static const char* const strStr = " string";

uint32_t Unicode::toCodePoint(const std::string &str) {
	if (
		!(0b10000000 & str[0]) && 1 == str.length()
	) {
		return str[0];
	} else if (
		(0b11000000 & str[0]) && !(0b00100000 & str[0]) &&
		2 == str.length() &&
		(0b10000000 & str[1]) && !(0b01000000 & str[1])
	) {
		return (((uint32_t)str[0] & 0b11111) << 6) | ((uint32_t)str[1] & 0b111111);
	} else if (
		(0b11100000 & str[0]) && !(0b00010000 & str[0]) &&
		3 == str.length() &&
		(0b10000000 & str[1]) && !(0b01000000 & str[1]) &&
		(0b10000000 & str[2]) && !(0b01000000 & str[2])
	) {
		return (((uint32_t)str[0] & 0b1111) << 12) | (((uint32_t)str[1] & 0b111111) << 6) | ((uint32_t)str[2] & 0b111111);
	} else if (
		(0b11110000 & str[0]) && !(0b00001000 & str[0]) &&
		4 == str.length() &&
		(0b10000000 & str[1]) && !(0b01000000 & str[1]) &&
		(0b10000000 & str[2]) && !(0b01000000 & str[2]) &&
		(0b10000000 & str[3]) && !(0b01000000 & str[3])
	) {
		return (((uint32_t)str[0] & 0b111) << 18) | (((uint32_t)str[1] & 0b111111) << 12) | (((uint32_t)str[2] & 0b111111) << 6) | ((uint32_t)str[3] & 0b111111);
	} else {
		return UINT32_MAX;
	}
}

uint32_t Unicode::toCodePoint(const std::u16string &str) {
	if (
		1 == str.length() &&
		(0xd800 >= str[0] || 0xe000 <= str[0])
	) {
		return str[0];
	} else if (
		2 == str.length() &&
		(0xd800 <= str[0] && 0xdbff >= str[0]) &&
		(0xdc00 <= str[1] && 0xdfff >= str[1])
	) {
		return ((((uint32_t)str[0] - 0xd800) << 10) | ((uint32_t)str[1] - 0xdc00)) + 0x10000;
	} else {
		return UINT32_MAX;
	}
}

uint32_t Unicode::toCodePoint(const char16_t word1, const char16_t word2) {
	if (
		(0xd800 >= word1 || 0xe000 <= word1) &&
		0 == word2
	) {
		return word1;
	} else if (
		(0xd800 <= word1 && 0xdbff >= word1) &&
		(0xdc00 <= word2 && 0xdfff >= word2)
	) {
		return ((((uint32_t)word1 - 0xd800) << 10) | ((uint32_t)word2 - 0xdc00)) + 0x10000;
	} else {
		return UINT32_MAX;
	}
}

std::string Unicode::toUtf8(uint32_t codePoint) {
	size_t count = 0;
	char str[4] = { 0 };
	if (0x80 > codePoint) {
		count = 1;
		str[0] = codePoint;
	} else if (0x80 <= codePoint && 0x800 > codePoint) {
		count = 2;
		str[0] = 0b11000000 | ((codePoint >> 6) & 0b00011111);
		str[1] = 0b10000000 | (codePoint & 0b00111111);
	} else if (0x800 <= codePoint && 0x10000 > codePoint) {
		count = 3;
		str[0] = 0b11100000 | ((codePoint >> 12) & 0b00001111);
		str[1] = 0b10000000 | ((codePoint >> 6) & 0b00111111);
		str[2] = 0b10000000 | (codePoint & 0b00111111);
	} else {
		count = 4;
		str[0] = 0b11110000 | ((codePoint >> 18) & 0b00000111);
		str[1] = 0b10000000 | ((codePoint >> 12) & 0b00111111);
		str[2] = 0b10000000 | ((codePoint >> 6) & 0b00111111);
		str[3] = 0b10000000 | (codePoint & 0b00111111);
	}
	return std::string(str, str + count);
}

std::string Unicode::toUtf8(const std::u16string &str) {
	auto codeUnits = str.c_str();
	std::string utf8Str = "";
	for (size_t i = 0; i < str.length();) {
		if (0xd800 >= codeUnits[i] || 0xe000 <= codeUnits[i]) {
			utf8Str += toUtf8(toCodePoint(codeUnits[i]));
			++i;
		} else if (
			(0xd800 <= codeUnits[i] && 0xdbff >= codeUnits[i]) &&
			(0xdc00 <= codeUnits[i + 1] && 0xdfff >= codeUnits[i + 1])
		) {
			utf8Str += toUtf8(toCodePoint(codeUnits[i], codeUnits[i + 1]));
			i += 2;
		} else {
			terminalPrintString(invalidStr, strlen(invalidStr));
			terminalPrintDecimal(16);
			terminalPrintString(strStr, strlen(strStr));
			Kernel::panic();
		}
	}
	return utf8Str;
}
