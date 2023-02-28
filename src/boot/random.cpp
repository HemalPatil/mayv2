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

void Random::GUIDv4::print() const {
	for (size_t i = 0; i < 8; ++i) {
		terminalPrintChar(hexPalette[(this->timeLow >> (28 - i * 4)) & 0xf]);
	}
	terminalPrintChar('-');
	for (size_t i = 0; i < 4; ++i) {
		terminalPrintChar(hexPalette[(this->timeMid >> (12 - i * 4)) & 0xf]);
	}
	terminalPrintChar('-');
	for (size_t i = 0; i < 4; ++i) {
		terminalPrintChar(hexPalette[(this->timeHighAndVersion >> (12 - i * 4)) & 0xf]);
	}
	terminalPrintChar('-');
	for (size_t i = 0; i < 4; ++i) {
		terminalPrintChar(hexPalette[(this->clockSequenceAndVariant >> (12 - i * 4)) & 0xf]);
	}
	terminalPrintChar('-');
	for (size_t i = 0; i < 12; ++i) {
		terminalPrintChar(hexPalette[(this->node >> (44 - i * 4)) & 0xf]);
	}
}
