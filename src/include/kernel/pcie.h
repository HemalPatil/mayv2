#pragma once

#include <cstdint>
#include <kernel.h>

#define PCI_CAPABILITIES_LIST_AVAILABLE (1 << 4)
#define PCI_MSI_CAPABAILITY_ID 0x5

#define PCI_CLASS_STORAGE 0x1
#define PCI_CLASS_BRIDGE 0x6

#define PCI_SUBCLASS_PCI_BRIDGE 0x4
#define PCI_SUBCLASS_PCI_BRIDGE2 0x9
#define PCI_SUBCLASS_SATA 0x6

#define PCI_PROG_AHCI 0x1

#define PCI_BUS_COUNT 256
#define PCI_DEVICE_COUNT 32
#define PCI_FUNCTION_COUNT 8
#define PCI_INVALID_DEVICE 0xffff
#define PCI_MULTI_FUNCTION_DEVICE 0x80

namespace PCIe {
	struct SegmentGroupEntry {
		uint64_t baseAddress;
		uint16_t groupNumber;
		uint8_t startBus;
		uint8_t endBus;
		uint32_t reserved;
	} __attribute__((packed));

	struct BaseHeader {
		uint16_t vendorId;
		uint16_t deviceId;
		uint16_t command;
		uint16_t status;
		uint8_t revisionId;
		uint8_t progIf;
		uint8_t subClass;
		uint8_t mainClass;
		uint8_t cacheLineSize;
		uint8_t latencyTimer;
		uint8_t headerType;
		uint8_t bist;
	} __attribute__((packed));

	struct Type0Header {
		BaseHeader baseHeader;
		uint32_t bar0;
		uint32_t bar1;
		uint32_t bar2;
		uint32_t bar3;
		uint32_t bar4;
		uint32_t bar5;
		uint32_t cardBusCis;
		uint16_t subsystemVendorId;
		uint16_t subsystemId;
		uint32_t expansionRomBaseAddress;
		uint8_t capabilities;
		uint8_t reserved0;
		uint16_t reserved1;
		uint32_t reserved2;
		uint8_t interruptLine;
		uint8_t interruptPin;
		uint8_t minGrant;
		uint8_t maxLatency;
	} __attribute__((packed));

	struct MSICapability {
		uint8_t capabilityId;
		uint8_t next;
		uint8_t enable : 1;
		uint8_t multiMessageCapable : 3;
		uint8_t multiMessageEnable : 3;
		uint8_t bit64Capable : 1;
		uint8_t perVectorMasking : 1;
		uint8_t reserved0 : 7;
		uint32_t messageAddress;
		uint16_t data;
		uint16_t reserved1;
		uint32_t mask;
		uint32_t pending;
	} __attribute__((packed));

	struct MSI64Capability {
		uint8_t capabilityId;
		uint8_t next;
		uint8_t enable : 1;
		uint8_t multiMessageCapable : 3;
		uint8_t multiMessageEnable : 3;
		uint8_t bit64Capable : 1;
		uint8_t perVectorMasking : 1;
		uint8_t reserved0 : 7;
		uint64_t messageAddress;
		uint16_t data;
		uint16_t reserved1;
		uint32_t mask;
		uint32_t pending;
	} __attribute__((packed));

	struct Function {
		uint8_t bus = 0xff;
		uint8_t device = 0xff;
		uint8_t function = 0xff;
		BaseHeader *configurationSpace = (BaseHeader*)INVALID_ADDRESS;
		MSICapability *msi = (MSICapability*)INVALID_ADDRESS;
	};

	extern std::vector<Function> functions;

	extern bool enumerate();
}
