#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APIC_TYPE_CPU 0
#define APIC_TYPE_IO 1
#define APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE 2

struct APICEntryHeader {
	uint8_t type;
	uint8_t length;
} __attribute__((packed));
typedef struct APICEntryHeader APICEntryHeader;

struct APICCPUEntry {
	APICEntryHeader header;
	uint8_t cpuId;
	uint8_t apicId;
	uint32_t flags;
} __attribute__((packed));
typedef struct APICCPUEntry APICCPUEntry;

struct APICIOEntry {
	APICEntryHeader header;
	uint8_t apicId;
	uint8_t reserved;
	uint32_t address;
	uint32_t globalInterruptBase;
} __attribute__((packed));
typedef struct APICIOEntry APICIOEntry;

struct APICInterruptSourceOverrideEntry {
	APICEntryHeader header;
	uint8_t busSource;
	uint8_t irqSource;
	uint32_t globalSystemInterrupt;
	uint16_t flags;
} __attribute__((packed));
typedef struct APICInterruptSourceOverrideEntry APICInterruptSourceOverrideEntry;

extern void disableLegacyPic();
extern size_t getCpuCount();
extern bool initializeApic();
extern bool isApicPresent();