#pragma once

#include <cstddef>
#include <cstdint>
#include <kernel.h>
#include <vector>

#define IOAPIC_READTBL_LOW(n) (0x10 + 2 * n)
#define IOAPIC_READTBL_HIGH(n) (0x10 + 2 * n + 1)

namespace APIC {
	enum EntryType : uint8_t {
		xAPIC = 0,
		IOAPIC = 1,
		InterruptSourceOverride = 2,
		x2APIC = 9,
	};

	struct EntryHeader {
		EntryType type;
		uint8_t length;
	} __attribute__((packed));

	struct xAPICEntry {
		EntryHeader header;
		uint8_t acpiId;
		uint8_t apicId;
		uint32_t flags;
	} __attribute__((packed));

	struct x2APICEntry {
		EntryHeader header;
		uint16_t reserved;
		uint32_t apicId;
		uint32_t flags;
		uint32_t acpiId;
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

	struct InterruptDataZone {
		uint32_t apicId;
		uint32_t reserved0;
		uint64_t reserved1;
		uint8_t floatSaveRegion[512];
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

	struct CPU {
		uint32_t apicId = UINT32_MAX;
		uint32_t flags = UINT32_MAX;
		uint64_t apicPhyAddr = UINT64_MAX;
		uint16_t tssSelector = 0;
		InterruptDataZone *intZone1 = nullptr;
		InterruptDataZone *intZone2 = nullptr;
	};

	extern CPU *bootCpu;
	extern std::vector<CPU> cpus;
	extern std::vector<InterruptSourceOverrideEntry> interruptOverrideEntries;
	extern std::vector<IOEntry> ioEntries;

	extern void acknowledgeLocalInterrupt();
	extern bool parse();
	extern uint32_t readIo(const uint8_t offset);
	extern IORedirectionEntry readIoRedirectionEntry(const Kernel::IRQ irq);
	extern void writeIo(const uint8_t offset, const uint32_t value);
	extern void writeIoRedirectionEntry(const Kernel::IRQ irq, const IORedirectionEntry entry);

	// apicasm.asm
	extern "C" void disableLegacyPic();
	extern "C" bool enableX2Apic();
}
