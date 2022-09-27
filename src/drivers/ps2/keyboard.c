#include <drivers/ps2/keyboard.h>
#include <interrupts.h>
#include <string.h>
#include <terminal.h>

static const char* const keyStr = "key";

void keyboardHandler() {
	terminalPrintString(keyStr, strlen(keyStr));
	acknowledgeLocalApicInterrupt();
}
