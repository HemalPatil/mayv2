#pragma once

#include <stdbool.h>
#include <stdint.h>

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

struct PCIeSegmentGroupEntry {
	uint64_t baseAddress;
	uint16_t groupNumber;
	uint8_t startBus;
	uint8_t endBus;
	uint32_t reserved;
} __attribute__((packed));
typedef struct PCIeSegmentGroupEntry PCIeSegmentGroupEntry;

struct PCIeBaseHeader {
	uint16_t vendorId;
	uint16_t deviceId;
	uint16_t command;
	uint16_t status;
	uint8_t revisionId;
	uint8_t progIf;
	uint8_t subClass;
	uint8_t class;
	uint8_t cacheLineSize;
	uint8_t latencyTimer;
	uint8_t headerType;
	uint8_t bist;
} __attribute__((packed));
typedef struct PCIeBaseHeader PCIeBaseHeader;

struct PCIeType0Header {
	PCIeBaseHeader baseHeader;
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
typedef struct PCIeType0Header PCIeType0Header;

// Assumes 64-bit addressing is available
struct PCIeMSI64Capability {
	uint8_t capabilityId;
	uint8_t next;
	uint8_t enable : 1;
	uint8_t multiMessageCapable : 3;
	uint8_t multiMessageEnable : 3;
	uint8_t bit64Capable : 1;
	uint8_t reserved0;
	uint64_t messageAddress;
	uint16_t data;
} __attribute__((packed));
typedef struct PCIeMSI64Capability PCIeMSI64Capability;

struct PCIeFunction {
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	PCIeBaseHeader *configurationSpace;
	PCIeMSI64Capability *msi64;
	struct PCIeFunction *next;
};
typedef struct PCIeFunction PCIeFunction;

extern PCIeFunction *pcieFunctions;

extern bool enumeratePCIe();