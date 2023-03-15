#include <drivers/console/output.h>

Drivers::Console::Output &Drivers::Console::kout = Drivers::Console::Output::getInstance();

Drivers::Console::Output& Drivers::Console::Output::getInstance() {
	return instance;
}

Drivers::Console::Output::Output() : mode(Mode::TextVga) {};

Drivers::Console::Output Drivers::Console::Output::instance;
