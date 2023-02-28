#include <random.h>
#include <terminal.h>

static const char* const hexPalette = "0123456789abcdef";

Random::GUIDv4::GUIDv4() {
	this->timeLow = getRandom64();
	this->timeMid = getRandom64();
	this->timeHighAndVersion = getRandom64();
	this->clockSequenceAndVariant = getRandom64();
	this->node = getRandom64();
	this->timeHighAndVersion &= 0xfff;
	this->timeHighAndVersion |= 0x4000;
	this->clockSequenceAndVariant &= 0x3fff;
	this->clockSequenceAndVariant |= 0x8000;
}

bool Random::GUIDv4::operator==(const GUIDv4 &other) const {
	return (
		this->timeLow == other.timeLow &&
		this->timeMid == other.timeMid &&
		this->timeHighAndVersion == other.timeHighAndVersion &&
		this->clockSequenceAndVariant == other.clockSequenceAndVariant &&
		(this->node & 0xffffffffffffUL) == (other.node & 0xffffffffffffUL)
	);
}

Random::GUIDv4::operator std::string() const {
	std::string toString = "00000000-0000-0000-0000-000000000000";
	size_t index = 0;
	for (size_t i = 0; i < 8; ++i, ++index) {
		toString[index] = hexPalette[(this->timeLow >> (28 - i * 4)) & 0xf];
	}
	++index;
	for (size_t i = 0; i < 4; ++i, ++index) {
		toString[index] = hexPalette[(this->timeMid >> (12 - i * 4)) & 0xf];
	}
	++index;
	for (size_t i = 0; i < 4; ++i, ++index) {
		toString[index] = hexPalette[(this->timeHighAndVersion >> (12 - i * 4)) & 0xf];
	}
	++index;
	for (size_t i = 0; i < 4; ++i, ++index) {
		toString[index] = hexPalette[(this->clockSequenceAndVariant >> (12 - i * 4)) & 0xf];
	}
	++index;
	for (size_t i = 0; i < 12; ++i, ++index) {
		toString[index] = hexPalette[(this->node >> (44 - i * 4)) & 0xf];
	}
	return toString;
}

void Random::GUIDv4::print() const {
	std::string toString = static_cast<std::string>(*this);
	terminalPrintString(toString.c_str(), 36);
}
