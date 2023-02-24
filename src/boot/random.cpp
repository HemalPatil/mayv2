#include <random.h>
#include <terminal.h>

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
	terminalPrintChar('{');
	terminalPrintHex(&this->timeLow, 4);
	terminalPrintChar('-');
	terminalPrintHex(&this->timeMid, 2);
	terminalPrintChar('-');
	terminalPrintHex(&this->timeHighAndVersion, 2);
	terminalPrintChar('-');
	terminalPrintHex(&this->clockSequenceAndVariant, 2);
	terminalPrintChar('-');
	terminalPrintHex(&this->node, 6);
	terminalPrintChar('}');
}
