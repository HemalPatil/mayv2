#pragma once

#include <stdbool.h>
#include <stdint.h>

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

struct PCIeConfigurationHeader {
	uint16_t vendorId;
	uint16_t deviceId;
	uint16_t command;
	uint16_t status;
	uint8_t revisionId;
	uint8_t progIf;
	uint8_t subclass;
	uint8_t class;
	uint8_t cacheLineSize;
	uint8_t latencyTimer;
	uint8_t headerType;
	uint8_t bist;
} __attribute__((packed));
typedef struct PCIeConfigurationHeader PCIeConfigurationHeader;

extern bool enumeratePCIe();