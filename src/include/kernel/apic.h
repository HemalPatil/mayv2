#pragma once

#include <cstddef>
#include <cstdint>
#include <kernel.h>
#include <vector>

#define APIC_TYPE_CPU 0
#define APIC_TYPE_IO 1
#define APIC_TYPE_INTERRUPT_SOURCE_OVERRIDE 2

#define LOCAL_APIC_DEFAULT_ADDRESS 0xfee00000

#define IOAPIC_READTBL_LOW(n) (0x10 + 2 * n)
#define IOAPIC_READTBL_HIGH(n) (0x10 + 2 * n + 1)

namespace APIC {
	struct EntryHeader {
		uint8_t type;
		uint8_t length;
	} __attribute__((packed));

	struct CPUEntry {
		EntryHeader header;
		uint8_t cpuId;
		uint8_t apicId;
		uint32_t flags;
	} __attribute__((packed));

	struct IOEntry {
		EntryHeader header;
		uint8_t apicId;
		uint8_t reserved;
		uint32_t address;
		uint32_t globalInterruptBase;
	} __attribute__((packed));

	struct InterruptSourceOverrideEntry {
		EntryHeader header;
		uint8_t busSource;
		uint8_t irqSource;
		uint32_t globalSystemInterrupt;
		uint16_t flags;
	} __attribute__((packed));

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
		char reserved11[396];
		uint32_t errorStatus;
		char reserved12[124];
		uint32_t interruptCommandLow;
		char reserved13[12];
		uint32_t interruptCommandHigh;
		char reserved14[12];
	} __attribute__((packed));

	union IORedirectionEntry {
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

	extern uint8_t bootCpuId;
	extern std::vector<CPUEntry> cpuEntries;
	extern std::vector<InterruptSourceOverrideEntry> interruptOverrideEntries;
	extern std::vector<IOEntry> ioEntries;

	extern void acknowledgeLocalInterrupt();
	extern volatile LocalAPIC* getLocal();
	extern bool initialize();
	extern uint32_t readIo(const uint8_t offset);
	extern IORedirectionEntry readIoRedirectionEntry(const Kernel::IRQ irq);
	extern void writeIo(const uint8_t offset, const uint32_t value);
	extern void writeIoRedirectionEntry(const Kernel::IRQ irq, const IORedirectionEntry entry);

	// apicasm.asm
	extern "C" void disableLegacyPic();
	extern "C" bool isApicPresent();
}
