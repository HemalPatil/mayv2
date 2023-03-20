#pragma once

#include <acpi.h>

namespace Drivers {
	namespace Timers {
		namespace HPET {
			enum TimerParamters : uint8_t {
				Enable = (1 << 2),
				Periodic = (1 << 3),
				PeriodicInterval = (1 << 6),
			};

			struct Info{
				ACPI::SDTHeader baseHeader;
				uint8_t hardwareRevisionId;
				uint8_t comparatorCount : 5;
				uint8_t counterSize : 1;
				uint8_t reserved0 : 1;
				uint8_t legacyReplacement : 1;
				uint16_t pciVendorId;
				uint8_t addressSpaceId;	// 0 - system memory, 1 - system I/O
				uint8_t registerBitWidth;
				uint8_t registerBitOffset;
				uint8_t reserved1;
				uint64_t address;
				uint8_t hpetNumber;
				uint16_t minimumTick;
				uint8_t pageProtection;
			} __attribute__((packed));

			struct Timer {
				uint8_t reserved0 : 1;
				uint8_t interruptType : 1;	// 0 - edge triggered, 1 - level triggered
				uint8_t interruptEnabled : 1;
				uint8_t periodicEnabled : 1;
				uint8_t periodicCapable : 1;
				uint8_t bit64Capable : 1;
				uint8_t valueSet : 1;
				uint8_t reserved1 : 1;
				uint8_t force32Bit : 1;
				uint8_t ioapicIrq : 5;
				uint8_t fsbEnabled : 1;
				uint8_t fsbCapable : 1;
				uint16_t reserved2;
				uint32_t ioapicAllowedIrqs;
				uint64_t comparatorValue;
				uint64_t fsbRoute;
				uint64_t reserved3;
			} __attribute__((packed));

			struct Registers {
				uint8_t revisionId;
				uint8_t timerCount: 5;	// number of timers - 1
				uint8_t bit64Capable : 1;
				uint8_t reserved0 : 1;
				uint8_t legacyMappingCapable : 1;
				uint16_t vendorId;
				uint32_t period;	// in femtoseconds (10^-15 seconds)
				uint64_t reserved1;
				uint8_t enabled : 1;
				uint8_t legacyMappingEnabled : 1;
				uint64_t reserved2 : 62;
				uint64_t reserved3;
				uint64_t interruptStatus;
				uint8_t reserved4[200];
				uint64_t mainCounterValue;
				uint64_t reserved5;
				Timer timers[32];
			} __attribute__((packed));

			extern Registers *registers;
			extern Timer *periodicTimer;
			extern void (*timerInterruptCallback)();

			extern bool initialize();
		}
	}
}

// hpet.cpp
extern "C" void hpetHandler();

// hpetasm.asm
extern "C" void hpetHandlerWrapper();
