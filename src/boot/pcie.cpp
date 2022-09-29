#include <commonstrings.h>
#include <heapmemmgmt.h>
#include <kernel.h>
#include <pcie.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static bool enumerateBus(uint64_t baseAddress, uint8_t bus);
static bool enumerateDevice(uint64_t baseAddress, uint8_t bus, uint8_t device);
static bool enumerateFunction(uint8_t function, PCIeBaseHeader *pcieHeader, PCIeMSICapability **msi);
static void* mapBDFPage(uint64_t baseAddress, uint8_t bus, uint8_t device, uint8_t function);

static const char* const initPciStr = "Enumerating PCIe devices";
static const char* const initPciCompleteStr = "PCIe enumeration complete\n\n";
static const char* const segmentStr = "Segment group ";
static const char* const busesStr = " buses ";
static const char* const baseStr = " base address ";
static const char* const busStr = "Bus ";
static const char* const deviceStr = " device ";
static const char* const multiStr = " multi function";
static const char* const funcStr = "Function ";
static const char* const nonMsiStr = "non-MSI";

static PCIeFunction *current = NULL;

PCIeFunction *pcieFunctions = NULL;

bool enumeratePCIe() {
	terminalPrintString(initPciStr, strlen(initPciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// FIXME: should verify the MCFG checksum
	// Create a first dummy entry that will be deleted later
	current = new PCIeFunction();
	current->configurationSpace = (PCIeBaseHeader*)INVALID_ADDRESS;
	current->next = NULL;
	pcieFunctions = current;
	size_t groupCount = (mcfgSdtHeader->length - sizeof(ACPISDTHeader) - sizeof(uint64_t)) / sizeof(PCIeSegmentGroupEntry);
	PCIeSegmentGroupEntry *groupEntries = (PCIeSegmentGroupEntry*)((uint64_t)mcfgSdtHeader + sizeof(ACPISDTHeader) + sizeof(uint64_t));
	for (size_t i = 0; i < groupCount; ++i) {
		terminalPrintSpaces4();
		terminalPrintString(segmentStr, strlen(segmentStr));
		terminalPrintDecimal(groupEntries[i].groupNumber);
		terminalPrintString(baseStr, strlen(baseStr));
		terminalPrintHex(&groupEntries[i].baseAddress, sizeof(groupEntries[i].baseAddress));
		terminalPrintString(busesStr, strlen(busesStr));
		terminalPrintDecimal(groupEntries[i].startBus);
		terminalPrintChar('-');
		terminalPrintDecimal(groupEntries[i].endBus);
		terminalPrintChar('\n');
		for (uint8_t bus = groupEntries[i].startBus; bus <= groupEntries[i].endBus; ++bus) {
			if (!enumerateBus(groupEntries[i].baseAddress, bus - groupEntries[i].startBus)) {
				return false;
			}
		}
	}
	// Delete the first dummy entry
	current = pcieFunctions;
	if (pcieFunctions->next) {
		pcieFunctions = pcieFunctions->next;
	} else {
		pcieFunctions = NULL;
	}
	delete current;

	terminalPrintString(initPciCompleteStr, strlen(initPciCompleteStr));
	return true;
}

static void* mapBDFPage(uint64_t baseAddress, uint8_t bus, uint8_t device, uint8_t function) {
	uint64_t functionAddress = baseAddress + (bus << 20 | device << 15 | function << 12);
	PageRequestResult requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
	if (
		requestResult.address != INVALID_ADDRESS &&
		requestResult.allocatedCount == 1 &&
		mapVirtualPages(requestResult.address, (void*) functionAddress, 1, MEMORY_REQUEST_CACHE_DISABLE)
	) {
		return requestResult.address;
	}
	return NULL;
}

static bool enumerateBus(uint64_t baseAddress, uint8_t bus) {
	for (uint8_t device = 0; device < PCI_DEVICE_COUNT; ++device) {
		if (!enumerateDevice(baseAddress, bus, device)) {
			return false;
		}
	}
	return true;
}

static bool enumerateFunction(uint8_t function, PCIeBaseHeader *pcieHeader, PCIeMSICapability **msi) {
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(funcStr, strlen(funcStr));
	terminalPrintDecimal(function);
	terminalPrintChar(' ');
	terminalPrintHex(&pcieHeader->mainClass, sizeof(pcieHeader->mainClass));
	terminalPrintChar(':');
	terminalPrintHex(&pcieHeader->subClass, sizeof(pcieHeader->subClass));
	terminalPrintChar(':');
	terminalPrintHex(&pcieHeader->progIf, sizeof(pcieHeader->progIf));
	if (pcieHeader->mainClass == PCI_CLASS_BRIDGE && pcieHeader->subClass == PCI_SUBCLASS_PCI_BRIDGE) {
		// FIXME: should enumerate secondary bus
	}

	// Enumerate all capabilities and find MSI64 (64-bit capable message signaled interrupt capability)
	if (pcieHeader->status & PCI_CAPABILITIES_LIST_AVAILABLE) {
		uint8_t capabilityOffset = ((PCIeType0Header*)pcieHeader)->capabilities;
		bool msiFound = false;
		while (capabilityOffset != 0) {
			uint8_t *capabilityId = (uint8_t*)((uint64_t)pcieHeader + capabilityOffset);
			if (*capabilityId == PCI_MSI_CAPABAILITY_ID) {
				*msi = (PCIeMSICapability*)capabilityId;
				msiFound = true;
				break;
			} else {
				capabilityOffset = *(capabilityId + 1);
			}
		}
		if (msiFound) {
			goto enumerateFunctionEnd;
		} else {
			goto msiNotFound;
		}
	}
	msiNotFound:
		*msi = NULL;
		terminalPrintChar(' ');
		terminalPrintString(nonMsiStr, strlen(nonMsiStr));

	enumerateFunctionEnd:
		terminalPrintChar('\n');
		return true;
}

static bool enumerateDevice(uint64_t baseAddress, uint8_t bus, uint8_t device) {
	uint8_t function = 0;
	PCIeBaseHeader *pcieHeader = (PCIeBaseHeader*) mapBDFPage(baseAddress, bus, device, function);
	if (!pcieHeader) {
		terminalPrintSpaces4();
		terminalPrintSpaces4();
		terminalPrintString(busStr, strlen(busStr));
		terminalPrintDecimal(bus);
		terminalPrintString(deviceStr, strlen(deviceStr));
		terminalPrintDecimal(device);
		terminalPrintChar(' ');
		terminalPrintString(failedStr, strlen(failedStr));
		return false;
	}
	if (pcieHeader->deviceId == PCI_INVALID_DEVICE) {
		// Unmap the configuration page
		freeVirtualPages(pcieHeader, 1, MEMORY_REQUEST_KERNEL_PAGE);
		return true;
	}
	terminalPrintSpaces4();
	terminalPrintSpaces4();
	terminalPrintString(busStr, strlen(busStr));
	terminalPrintDecimal(bus);
	terminalPrintString(deviceStr, strlen(deviceStr));
	terminalPrintDecimal(device);
	size_t x, y;
	terminalGetCursorPosition(&x, &y);
	terminalSetCursorPosition(24, y);
	terminalPrintHex(&pcieHeader->vendorId, sizeof(pcieHeader->vendorId));
	terminalPrintChar(':');
	terminalPrintHex(&pcieHeader->deviceId, sizeof(pcieHeader->deviceId));
	if (pcieHeader->headerType & PCI_MULTI_FUNCTION_DEVICE) {
		terminalPrintString(multiStr, strlen(multiStr));
	}
	terminalPrintChar('\n');
	PCIeMSICapability *msi;
	if (!enumerateFunction(function, pcieHeader, &msi)) {
		return false;
	}
	current->next = new PCIeFunction();
	current = current->next;
	current->bus = bus;
	current->device = device;
	current->function = function;
	current->configurationSpace = pcieHeader;
	current->msi = msi;
	current->next = NULL;
	if (pcieHeader->headerType & PCI_MULTI_FUNCTION_DEVICE) {
		// TODO: complete multi function device enumeration
		for (function = 1; function < PCI_FUNCTION_COUNT; ++function) {
			pcieHeader = (PCIeBaseHeader*) mapBDFPage(baseAddress, bus, device, function);
			if (!pcieHeader) {
				terminalPrintSpaces4();
				terminalPrintSpaces4();
				terminalPrintSpaces4();
				terminalPrintString(funcStr, strlen(funcStr));
				terminalPrintDecimal(function);
				terminalPrintChar(' ');
				terminalPrintString(failedStr, strlen(failedStr));
				return false;
			}
			if (pcieHeader->deviceId == PCI_INVALID_DEVICE) {
				// Unmap the configuration page
				freeVirtualPages(pcieHeader, 1, MEMORY_REQUEST_KERNEL_PAGE);
				continue;
			}
			if (!enumerateFunction(function, pcieHeader, &msi)) {
				return false;
			}
			current->next = new PCIeFunction();
			current = current->next;
			current->bus = bus;
			current->device = device;
			current->function = function;
			current->configurationSpace = pcieHeader;
			current->msi = msi;
			current->next = NULL;
		}
	}
	return true;
}
