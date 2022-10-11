#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APIC_TYPE_CPU 0
#define APIC_TYPE_IO 1
#define APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE 2

#define LOCAL_APIC_DEFAULT_ADDRESS 0xfee00000

#define IOAPIC_ID 0x00
#define IOAPIC_VER 0x01
#define IOAPIC_ARB 0x02
#define IOAPIC_READTBL_LOW(n) (0x10 + 2 * n)
#define IOAPIC_READTBL_HIGH(n) (0x10 + 2 * n + 1)

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

struct LocalAPIC {
	char reserved1[32];
	uint32_t apicId;
	char reserved2[12];
	uint32_t version;
	char reserved3[76];
	uint32_t taskPriority;
	char reserved4[12];
	uint32_t arbitrationPriority;
	char reserved5[12];
	uint32_t cpuPriority;
	char reserved6[12];
	uint32_t endOfInterrupt;
	char reserved7[12];
	uint32_t remoteRead;
	char reserved8[12];
	uint32_t logicalDestination;
	char reserved9[12];
	uint32_t destinationFormat;
	char reserved10[12];
	uint32_t spuriousInterruptVector;
	char reserved11[12];
} __attribute__((packed));
typedef struct LocalAPIC LocalAPIC;

union IOAPICRedirectionEntry {
	struct {
		uint64_t vector : 8;
		uint64_t deliveryMode : 3;
		uint64_t destinationMode : 1;
		uint64_t deliveryStatus : 1;
		uint64_t pinPolarity : 1;
		uint64_t remoteIRR : 1;
		uint64_t triggerMode : 1;
		uint64_t mask : 1;
		uint64_t reserved : 39;
		uint64_t destination : 8;
	} __attribute__((packed));

	struct {
		uint32_t lowDword;
		uint32_t highDword;
	} __attribute__((packed));
};
typedef union IOAPICRedirectionEntry IOAPICRedirectionEntry;

// apic.cpp
extern uint8_t bootCpu;

extern void acknowledgeLocalApicInterrupt();
extern size_t getCpuCount();
extern LocalAPIC* getLocalApic();
extern bool initializeApic();
extern uint32_t readIoApic(const uint8_t offset);
extern IOAPICRedirectionEntry readIoRedirectionEntry(const uint8_t irq);
extern void writeIoApic(const uint8_t offset, const uint32_t value);
extern void writeIoRedirectionEntry(const uint8_t irq, const IOAPICRedirectionEntry entry);

// apicasm.asm
extern "C" void disableLegacyPic();
extern "C" bool isApicPresent();
