#include <commonstrings.h>
#include <drivers/storage/ahci.h>
#include <string.h>
#include <terminal.h>

static const char* const initAhciStr = "Initializing AHCI";

bool initializeAHCI(PCIeFunction *pcieFunction) {
	terminalPrintString(initAhciStr, strlen(initAhciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	return true;
}