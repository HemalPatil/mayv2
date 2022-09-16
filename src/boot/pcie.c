#include <commonstrings.h>
#include <kernel.h>
#include <pcie.h>
#include <string.h>
#include <terminal.h>
#include <virtualmemmgmt.h>

static const char* const initPciStr = "Enumerating PCIe devices";
static const char* const initPciCompleteStr = "PCIe enumeration complete";
static const char* const segmentStr = "Segment group ";
static const char* const busesStr = " buses ";
static const char* const baseStr = " base address ";
static const char* const busStr = "Bus ";
static const char* const deviceStr = " device ";
static const char* const multiStr = " multi";

bool enumeratePCIe() {
	terminalPrintString(initPciStr, strlen(initPciStr));
	terminalPrintString(ellipsisStr, strlen(ellipsisStr));
	terminalPrintChar('\n');

	// TODO: complete PCI enumeration
	// FIXME: should verify the MCFG checksum
	size_t groupCount = (mcfg->length - sizeof(ACPISDTHeader) - sizeof(uint64_t)) / sizeof(PCIeSegmentGroupEntry);
	PCIeSegmentGroupEntry *groupEntries = (PCIeSegmentGroupEntry*)((uint64_t)mcfg + sizeof(ACPISDTHeader) + sizeof(uint64_t));
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
		for (size_t bus = groupEntries[i].startBus; bus <= groupEntries[i].endBus; ++bus) {
			for (size_t device = 0; device < PCI_DEVICE_COUNT; ++device) {
				uint8_t function = 0;
				uint64_t functionAddress = groupEntries[i].baseAddress + ((bus - groupEntries[i].startBus) << 20 | device << 15 | function << 12);
				PageRequestResult requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
				if (
					requestResult.address == INVALID_ADDRESS ||
					requestResult.allocatedCount != 1 ||
					!mapVirtualPages(requestResult.address, (void*) functionAddress, 1)
				) {
					terminalPrintSpaces4();
					terminalPrintSpaces4();
					terminalPrintString("Bus ", 4);
					terminalPrintDecimal(bus);
					terminalPrintString(" device ", 8);
					terminalPrintDecimal(device);
					terminalPrintChar(' ');
					terminalPrintString(failedStr, strlen(failedStr));
					terminalPrintChar('\n');
					return false;
				}

				PCIeConfigurationHeader *pcieHeader = (PCIeConfigurationHeader*) requestResult.address;
				if (pcieHeader->deviceId == PCI_INVALID_DEVICE) {
					// Unmap the configuration page
					freeVirtualPages(requestResult.address, 1, MEMORY_REQUEST_KERNEL_PAGE);
					continue;
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
					// TODO: complete multi function device enumeration
					terminalPrintString(multiStr, strlen(multiStr));
					// terminalPrintChar('\n');
					// for (function = 1; function < PCI_FUNCTION_COUNT; ++function) {
					// 	functionAddress = groupEntries[i].baseAddress + ((bus - groupEntries[i].startBus) << 20 | device << 15 | function << 12);
					// 	requestResult = requestVirtualPages(1, MEMORY_REQUEST_KERNEL_PAGE | MEMORY_REQUEST_CONTIGUOUS);
					// 	if (
					// 		requestResult.address == INVALID_ADDRESS ||
					// 		requestResult.allocatedCount != 1 ||
					// 		!mapVirtualPages(requestResult.address, (void*) functionAddress, 1)
					// 	) {
					// 		terminalPrintSpaces4();
					// 		terminalPrintSpaces4();
					// 		terminalPrintSpaces4();
					// 		terminalPrintString("Bus ", 4);
					// 		terminalPrintDecimal(bus);
					// 		terminalPrintString(" device ", 8);
					// 		terminalPrintDecimal(device);
					// 		terminalPrintChar(' ');
					// 		terminalPrintString(failedStr, strlen(failedStr));
					// 		terminalPrintChar('\n');
					// 		return false;
					// 	}
					// 	pcieHeader = (PCIeConfigurationHeader*) requestResult.address;
					// 	if (pcieHeader->deviceId == PCI_INVALID_DEVICE) {
					// 		// Unmap the configuration page
					// 		// freeVirtualPages(requestResult.address, 1, MEMORY_REQUEST_KERNEL_PAGE);
					// 		// hangSystem(true);
					// 		continue;
					// 	}
					// }
				}
				terminalPrintChar('\n');
			}
		}
	}
	hangSystem(true);

	terminalPrintString(initPciCompleteStr, strlen(initPciCompleteStr));
	return true;
}