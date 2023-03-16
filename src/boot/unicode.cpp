#include <cstring>
#include <kernel.h>
#include <terminal.h>
#include <unicode.h>

static const char* const invalidStr = "Invalid UTF-";
static const char* const strStr = " string";

static const uint8_t utf8Last7Mask = 0b01111111;
static const uint8_t utf8Last7MaskInv = ~utf8Last7Mask;		// 0b10000000
static const uint8_t utf8Last6Mask = utf8Last7Mask >> 1;	// 0b00111111
static const uint8_t utf8Last6MaskInv = ~utf8Last6Mask;		// 0b11000000
static const uint8_t utf8Last5Mask = utf8Last6Mask >> 1;	// 0b00011111
static const uint8_t utf8Last5MaskInv = ~utf8Last5Mask;		// 0b11100000
static const uint8_t utf8Last4Mask = utf8Last5Mask >> 1;	// 0b00001111
static const uint8_t utf8Last4MaskInv = ~utf8Last4Mask;		// 0b11110000
static const uint8_t utf8Last3Mask = utf8Last4Mask >> 1;	// 0b00000111
static const uint8_t utf8Last3MaskInv = ~utf8Last3Mask;		// 0b11111000
static const uint32_t pointUtf8Plane1Start = 0;
static const uint32_t pointUtf8Plane2Start = 0x80;
static const uint32_t pointUtf8Plane3Start = 0x800;
static const uint32_t pointUtf8Plane4Start = 0x10000;
static const uint32_t pointEnd = 0x110000;

uint32_t Unicode::toCodePoint(const std::string &str) {
	if (
		((utf8Last7MaskInv & str[0]) == 0) &&
		1 == str.length()
	) {
		return str[0];
	} else if (
		((utf8Last5MaskInv & str[0]) == utf8Last6MaskInv) &&
		2 == str.length() &&
		((utf8Last6MaskInv & str[1]) == utf8Last7MaskInv)
	) {
		return (((uint32_t)str[0] & utf8Last5Mask) << 6) | ((uint32_t)str[1] & utf8Last6Mask);
	} else if (
		((utf8Last4MaskInv & str[0]) == utf8Last5MaskInv) &&
		3 == str.length() &&
		((utf8Last6MaskInv & str[1]) == utf8Last7MaskInv) &&
		((utf8Last6MaskInv & str[2]) == utf8Last7MaskInv)
	) {
		return (((uint32_t)str[0] & utf8Last4Mask) << 12) | (((uint32_t)str[1] & utf8Last6Mask) << 6) | ((uint32_t)str[2] & utf8Last6Mask);
	} else if (
		((utf8Last3MaskInv & str[0]) == utf8Last4MaskInv) &&
		4 == str.length() &&
		((utf8Last6MaskInv & str[1]) == utf8Last7MaskInv) &&
		((utf8Last6MaskInv & str[2]) == utf8Last7MaskInv) &&
		((utf8Last6MaskInv & str[3]) == utf8Last7MaskInv)
	) {
		return (((uint32_t)str[0] & utf8Last3Mask) << 18) | (((uint32_t)str[1] & utf8Last6Mask) << 12) | (((uint32_t)str[2] & utf8Last6Mask) << 6) | ((uint32_t)str[3] & utf8Last6Mask);
	} else {
		return UINT32_MAX;
	}
}

bool Unicode::isValidUtf8(const std::string &str) {
	const auto length = str.length();
	if (length == 0) {
		return true;
	}
	for (size_t i = 0; i < length;) {
		if ((str[i] & utf8Last7MaskInv) == 0) {
			++i;
		} else if (
			((utf8Last5MaskInv & str[i]) == utf8Last6MaskInv) &&
			((i + 1) < length) &&
			((utf8Last6MaskInv & str[i + 1]) == utf8Last7MaskInv)
		) {
			i += 2;
		} else if (
			((utf8Last4MaskInv & str[i]) == utf8Last5MaskInv) &&
			((i + 2) < length) &&
			((utf8Last6MaskInv & str[i + 1]) == utf8Last7MaskInv) &&
			((utf8Last6MaskInv & str[i + 2]) == utf8Last7MaskInv)
		) {
			i += 3;
		} else if (
			((utf8Last3MaskInv & str[i]) == utf8Last4MaskInv) &&
			((i + 3) < length) &&
			((utf8Last6MaskInv & str[i + 1]) == utf8Last7MaskInv) &&
			((utf8Last6MaskInv & str[i + 2]) == utf8Last7MaskInv) &&
			((utf8Last6MaskInv & str[i + 3]) == utf8Last7MaskInv)
		) {
			i += 4;
		} else {
			return false;
		}
	}
	return true;
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
	if (codePoint >= pointUtf8Plane1Start && codePoint < pointUtf8Plane2Start) {
		count = 1;
		str[0] = codePoint;
	} else if (codePoint >= pointUtf8Plane2Start && codePoint < pointUtf8Plane3Start) {
		count = 2;
		str[0] = utf8Last6MaskInv | ((codePoint >> 6) & utf8Last5Mask);
		str[1] = utf8Last7MaskInv | (codePoint & utf8Last6Mask);
	} else if (codePoint >= pointUtf8Plane3Start && codePoint < pointUtf8Plane4Start) {
		count = 3;
		str[0] = utf8Last5MaskInv | ((codePoint >> 12) & utf8Last4Mask);
		str[1] = utf8Last7MaskInv | ((codePoint >> 6) & utf8Last6Mask);
		str[2] = utf8Last7MaskInv | (codePoint & utf8Last6Mask);
	} else if (codePoint >= pointUtf8Plane4Start && codePoint < pointEnd) {
		count = 4;
		str[0] = utf8Last4MaskInv | ((codePoint >> 18) & utf8Last3Mask);
		str[1] = utf8Last7MaskInv | ((codePoint >> 12) & utf8Last6Mask);
		str[2] = utf8Last7MaskInv | ((codePoint >> 6) & utf8Last6Mask);
		str[3] = utf8Last7MaskInv | (codePoint & utf8Last6Mask);
	} else {
		return "";
	}
	return std::string(str, str + count);
}

std::string Unicode::toUtf8(const std::u16string &str) {
	std::string utf8Str = "";
	for (size_t i = 0; i < str.length();) {
		if (0xd800 >= str[i] || 0xe000 <= str[i]) {
			utf8Str += toUtf8(toCodePoint(str[i]));
			++i;
		} else if (
			(0xd800 <= str[i] && 0xdbff >= str[i]) &&
			(0xdc00 <= str[i + 1] && 0xdfff >= str[i + 1])
		) {
			utf8Str += toUtf8(toCodePoint(str[i], str[i + 1]));
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
