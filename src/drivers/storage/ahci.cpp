#include <apic.h>
#include <commonstrings.h>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <heapmemmgmt.h>
#include <idt64.h>
#include <string.h>
#include <terminal.h>

static const char* const initAhciStr = "Initializing AHCI controller ";
static const char* const initAhciCompleteStr = "AHCI controller initialized\n\n";
static const char* const checkMsiStr = "Checking MSI capability";
static const char* const msiInstallingStr = "Installing AHCI MSI handler";

static size_t msiInterrupt = SIZE_MAX;
static size_t controllerCount = 0;

AHCI::Controller *AHCI::controllers = nullptr;

void ahciMsiHandler() {
	acknowledgeLocalApicInterrupt();
	AHCI::Controller *currentController = AHCI::controllers;
	terminalPrintString("msi ", 5);
	while (currentController) {
		uint32_t interruptStatus = currentController->hba->interruptStatus;
		if (interruptStatus) {
			// If any port needs servicing write its value back to hba->interruptStatus
			currentController->hba->interruptStatus = interruptStatus;
			terminalPrintString("msiack ", 7);
			for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
				if (
					interruptStatus & ((uint32_t)1 << i) &&
					currentController->devices[i]
				) {
					currentController->devices[i]->msiHandler();
				}
			}
		}
		currentController = currentController->next;
	}
}

bool AHCI::initialize(PCIeFunction *pcieFunction) {
	terminalPrintString(initAhciStr, strlen(initAhciStr));
	terminalPrintDecimal(controllerCount);
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Install AHCI MSI handler if not already installed
	if (msiInterrupt == SIZE_MAX) {
		terminalPrintSpaces4();
		terminalPrintString(msiInstallingStr, strlen(msiInstallingStr));
		terminalPrintString(ellipsisStr, strlen(ellipsisStr));
		msiInterrupt = availableInterrupt;
		terminalPrintHex(&msiInterrupt, sizeof(msiInterrupt));
		installIdt64Entry(msiInterrupt, &ahciMsiHandlerWrapper);
		++availableInterrupt;
		terminalPrintString(doneStr, strlen(doneStr));
		terminalPrintChar('\n');
	}

	// Verify the MSI64 capability
	terminalPrintSpaces4();
	terminalPrintString(checkMsiStr, strlen(checkMsiStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!pcieFunction->msi) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		return false;
	}
	if (pcieFunction->msi->bit64Capable) {
		PCIeMSI64Capability *msi64 = (PCIeMSI64Capability*)pcieFunction->msi;
		msi64->messageAddress = LOCAL_APIC_DEFAULT_ADDRESS;
		msi64->data = msiInterrupt;
		msi64->enable = 1;
	} else {
		pcieFunction->msi->messageAddress = LOCAL_APIC_DEFAULT_ADDRESS;
		pcieFunction->msi->data = msiInterrupt;
		pcieFunction->msi->enable = 1;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Create new controller, add to the list, and initialize it
	Controller *currentController;
	if (!controllers) {
		currentController = controllers = new Controller();
	} else {
		currentController = controllers;
		while (currentController->next) {
			currentController = currentController->next;
		}
		currentController->next = new Controller();
		currentController = currentController->next;
	}
	bool result = currentController->initialize(pcieFunction);
	if (result) {
		terminalPrintString(initAhciCompleteStr, strlen(initAhciCompleteStr));
		++controllerCount;
	}
	return result;
}
