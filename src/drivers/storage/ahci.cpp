#include <apic.h>
#include <commonstrings.h>
#include <cstring>
#include <drivers/storage/ahci.h>
#include <drivers/storage/ahci/controller.h>
#include <drivers/storage/ahci/device.h>
#include <idt64.h>
#include <terminal.h>

static const char* const initAhciStr = "Initializing AHCI controller ";
static const char* const initAhciCompleteStr = "AHCI controller initialized\n\n";
static const char* const checkMsiStr = "Checking MSI capability";
static const char* const msiInstallingStr = "Installing AHCI MSI handler";

static size_t msiInterrupt = SIZE_MAX;

std::vector<AHCI::Controller> AHCI::controllers;

void ahciMsiHandler() {
	APIC::acknowledgeLocalInterrupt();
	for (auto &controller : AHCI::controllers) {
		uint32_t interruptStatus = controller.hba->interruptStatus;
		if (interruptStatus) {
			// If any port needs servicing write its value back to hba->interruptStatus
			// to acknowledge the interrupt
			controller.hba->interruptStatus = interruptStatus;
			for (size_t i = 0; i < AHCI_PORT_COUNT; ++i) {
				if (
					interruptStatus & ((uint32_t)1 << i) &&
					controller.devices[i]
				) {
					controller.devices[i]->msiHandler();
				}
			}
		}
	}
}

Kernel::Async::Thenable<bool> AHCI::initialize(PCIe::Function &pcieFunction) {
	terminalPrintString(initAhciStr, strlen(initAhciStr));
	terminalPrintDecimal(AHCI::controllers.size());
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// Install AHCI MSI handler if not already installed
	if (msiInterrupt == SIZE_MAX) {
		terminalPrintSpaces4();
		terminalPrintString(msiInstallingStr, strlen(msiInstallingStr));
		terminalPrintString(ellipsisStr, strlen(ellipsisStr));
		msiInterrupt = availableInterrupt;
		installIdt64Entry(msiInterrupt, &ahciMsiHandlerWrapper);
		++availableInterrupt;
		terminalPrintString(doneStr, strlen(doneStr));
		terminalPrintChar('\n');
	}

	// Verify the MSI capability and install msiInterrupt
	terminalPrintSpaces4();
	terminalPrintString(checkMsiStr, strlen(checkMsiStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	if (!pcieFunction.msi) {
		terminalPrintString(notStr, strlen(notStr));
		terminalPrintChar(' ');
		terminalPrintString(okStr, strlen(okStr));
		terminalPrintChar('\n');
		co_return false;
	}
	if (pcieFunction.msi->bit64Capable) {
		PCIe::MSI64Capability *msi64 = (PCIe::MSI64Capability*)pcieFunction.msi;
		msi64->messageAddress = LOCAL_APIC_DEFAULT_ADDRESS;
		msi64->data = msiInterrupt;
		msi64->enable = 1;
	} else {
		pcieFunction.msi->messageAddress = LOCAL_APIC_DEFAULT_ADDRESS;
		pcieFunction.msi->data = msiInterrupt;
		pcieFunction.msi->enable = 1;
	}
	terminalPrintString(okStr, strlen(okStr));
	terminalPrintChar('\n');

	// Create new controller, add it to the list, and initialize it
	AHCI::controllers.push_back(AHCI::Controller());
	bool result = co_await AHCI::controllers.back().initialize(pcieFunction);
	if (result) {
		terminalPrintString(initAhciCompleteStr, strlen(initAhciCompleteStr));
	}
	co_return result;
}
