#include <commonstrings.h>
#include <cstring>
#include <kernel.h>
#include <pcie.h>
#include <terminal.h>

#define PCI_CAPABILITIES_LIST_AVAILABLE (1 << 4)
#define PCI_MSI_CAPABAILITY_ID 0x5

#define PCI_BUS_COUNT 256
#define PCI_DEVICE_COUNT 32
#define PCI_FUNCTION_COUNT 8
#define PCI_INVALID_DEVICE 0xffff
#define PCI_MULTI_FUNCTION_DEVICE 0x80

static bool enumerateBus(uint64_t baseAddress, uint8_t bus);
static bool enumerateDevice(uint64_t baseAddress, uint8_t bus, uint8_t device);
static bool enumerateFunction(uint8_t function, PCIe::BaseHeader *pcieHeader, PCIe::MSICapability **msi);
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
static const char* const unmapFailStr = "Failed to unmap configuration page";

std::vector<PCIe::Function> PCIe::functions;

bool PCIe::enumerate() {
	terminalPrintString(initPciStr, strlen(initPciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// FIXME: should verify the MCFG checksum
	size_t groupCount = (ACPI::mcfgSdtHeader->length - sizeof(ACPI::SDTHeader) - sizeof(uint64_t)) / sizeof(SegmentGroupEntry);
	SegmentGroupEntry *groupEntries = (SegmentGroupEntry*)((uint64_t)ACPI::mcfgSdtHeader + sizeof(ACPI::SDTHeader) + sizeof(uint64_t));
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

	terminalPrintString(initPciCompleteStr, strlen(initPciCompleteStr));
	return true;
}

static void* mapBDFPage(uint64_t baseAddress, uint8_t bus, uint8_t device, uint8_t function) {
	using namespace Kernel::Memory;

	uint64_t functionAddress = baseAddress + (bus << 20 | device << 15 | function << 12);
	PageRequestResult requestResult = Virtual::requestPages(
		1,
		(
			RequestType::Kernel |
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous
		)
	);
	if (
		requestResult.address != INVALID_ADDRESS &&
		requestResult.allocatedCount == 1 &&
		Virtual::mapPages(
			requestResult.address,
			(void*) functionAddress,
			1,
			RequestType::CacheDisable
		)
	) {
		return requestResult.address;
	}
	return nullptr;
}

static bool enumerateBus(uint64_t baseAddress, uint8_t bus) {
	for (uint8_t device = 0; device < PCI_DEVICE_COUNT; ++device) {
		if (!enumerateDevice(baseAddress, bus, device)) {
			return false;
		}
	}
	return true;
}

static bool enumerateFunction(uint8_t function, PCIe::BaseHeader *pcieHeader, PCIe::MSICapability **msi) {
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
	if (pcieHeader->mainClass == PCIe::Class::Bridge && pcieHeader->subClass == PCIe::Subclass::PciBridge) {
		// FIXME: should enumerate secondary bus
	}

	// Enumerate all capabilities and find MSI64 (64-bit capable message signaled interrupt capability)
	if (pcieHeader->status & PCI_CAPABILITIES_LIST_AVAILABLE) {
		uint8_t capabilityOffset = ((PCIe::Type0Header*)pcieHeader)->capabilities;
		bool msiFound = false;
		while (capabilityOffset != 0) {
			uint8_t *capabilityId = (uint8_t*)((uint64_t)pcieHeader + capabilityOffset);
			if (*capabilityId == PCI_MSI_CAPABAILITY_ID) {
				*msi = (PCIe::MSICapability*)capabilityId;
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
		*msi = nullptr;
		terminalPrintChar(' ');
		terminalPrintString(nonMsiStr, strlen(nonMsiStr));

	enumerateFunctionEnd:
		terminalPrintChar('\n');
		return true;
}

static bool enumerateDevice(uint64_t baseAddress, uint8_t bus, uint8_t device) {
	using namespace Kernel::Memory;

	uint8_t function = 0;
	PCIe::BaseHeader *pcieHeader = (PCIe::BaseHeader*) mapBDFPage(baseAddress, bus, device, function);
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
		if (!Virtual::freePages(pcieHeader, 1, RequestType::Kernel)) {
			terminalPrintSpaces4();
			terminalPrintString(unmapFailStr, strlen(unmapFailStr));
			Kernel::panic();
		}
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
	PCIe::MSICapability *msi;
	if (!enumerateFunction(function, pcieHeader, &msi)) {
		return false;
	}
	PCIe::Function currentFunction;
	currentFunction.bus = bus;
	currentFunction.device = device;
	currentFunction.function = function;
	currentFunction.configurationSpace = pcieHeader;
	currentFunction.msi = msi;
	PCIe::functions.push_back(currentFunction);
	if (pcieHeader->headerType & PCI_MULTI_FUNCTION_DEVICE) {
		for (function = 1; function < PCI_FUNCTION_COUNT; ++function) {
			pcieHeader = (PCIe::BaseHeader*) mapBDFPage(baseAddress, bus, device, function);
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
				if (!Virtual::freePages(pcieHeader, 1, RequestType::Kernel)) {
					terminalPrintSpaces4();
					terminalPrintString(unmapFailStr, strlen(unmapFailStr));
					Kernel::panic();
				}
				continue;
			}
			if (!enumerateFunction(function, pcieHeader, &msi)) {
				return false;
			}
			currentFunction.bus = bus;
			currentFunction.device = device;
			currentFunction.function = function;
			currentFunction.configurationSpace = pcieHeader;
			currentFunction.msi = msi;
			PCIe::functions.push_back(currentFunction);
		}
	}
	return true;
}
