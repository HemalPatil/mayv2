#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <pcie.h>

#define ATAPI_READTOC 0xa8

#define AHCI_COMMAND_LIST_SIZE 1024
#define AHCI_COMMAND_READ_DMA_EX 0x25
#define AHCI_COMMAND_ATAPI_PACKET 0xa0

#define AHCI_DEVICE_BUSY 0x80
#define AHCI_DEVICE_DRQ 0x08

#define AHCI_FIS_TYPE_REG_D2H 0x34
#define AHCI_FIS_TYPE_REG_H2D 0x27

#define AHCI_PORT_ACTIVE 1
#define AHCI_PORT_COUNT 32
#define AHCI_PORT_DEVICE_PRESENT 3

#define AHCI_PORT_SIGNATURE_SATA 0x00000101
#define AHCI_PORT_SIGNATURE_SATAPI 0xeb140101

#define AHCI_PORT_TYPE_NONE 0
#define AHCI_PORT_TYPE_SATA 1
#define AHCI_PORT_TYPE_SATAPI 4

#define AHCI_SECTOR_SIZE 512

#define AHCI_MODE ((uint32_t)1 << 31)
#define AHCI_GHC_INTERRUPT_ENABLE ((uint32_t)1 << 1)
#define AHCI_TASK_FILE_ERROR ((uint32_t)1 << 30)

// Refer AHCI spec https://www.intel.com.au/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
struct AHCIPortStatus {
	uint8_t deviceDetection : 4;
	uint8_t interfaceSpeed : 4;
	uint8_t powerMgmt : 4;
	uint8_t reserved0 : 4;
	uint16_t reserved1;
} __attribute__((packed));
typedef struct AHCIPortStatus AHCIPortStatus;

struct AHCIPortCommandStatus {
	uint8_t start : 1;
	uint8_t spinUp : 1;
	uint8_t powerOn : 1;
	uint8_t commandListOverride : 1;
	uint8_t fisReceiveEnable : 1;
	uint8_t reserved0 : 3;
	uint8_t currentCommandSlot : 5;
	uint8_t mechanicalPresenceSwitch : 1;
	uint8_t fisReceiveRunning : 1;
	uint8_t commandListRunning : 1;
	uint8_t coldPresence : 1;
	uint8_t portMultiplierAttached : 1;
	uint8_t hotPlugCapable : 1;
	uint8_t mechanicalPresenceSwitchAttached : 1;
	uint8_t coldPresenceDetection : 1;
	uint8_t external : 1;
	uint8_t fisSwitchingCapable : 1;
	uint8_t autoSlumberTransition : 1;
	uint8_t isAtapi : 1;
	uint8_t ledAtapi : 1;
	uint8_t aggressivePowerMgmt : 1;
	uint8_t aggressiveSlumber : 1;
	uint8_t interfaceCommunicationControl : 4;
} __attribute__((packed));
typedef struct AHCIPortCommandStatus AHCIPortCommandStatus;

struct AHCIPort {
	uint32_t commandListBase;
	uint32_t commandListBaseUpper;
	uint32_t fisBase;
	uint32_t fisBaseUpper;
	uint32_t interruptStatus;
	uint32_t interruptEnable;
	AHCIPortCommandStatus commandStatus;
	uint32_t reserved0;
	uint32_t taskFileData;
	uint32_t signature;
	AHCIPortStatus sataStatus;
	uint32_t sataControl;
	uint32_t sataError;
	uint32_t sataActive;
	uint32_t commandIssue;
	uint32_t sataNotification;
	uint32_t fisSwitchControl;
	uint32_t deviceSleep;
	uint8_t reserved1[40];
	uint32_t vendor[4];
} __attribute__((packed));
typedef struct AHCIPort AHCIPort;

struct AHCIHostCapabilities {
	uint8_t portCount : 5;
	uint8_t externalSataSupport : 1;
	uint8_t enclosureMgmtSupport : 1;
	uint8_t cccSupport : 1;
	uint8_t commandSlotsCount : 5;
	uint8_t partialCapable : 1;
	uint8_t slumberCapable : 1;
	uint8_t pioMultipleDrqBlock : 1;
	uint8_t fisSwitchingSupport : 1;
	uint8_t portMultiplierSupport : 1;
	uint8_t ahciOnly : 1;
	uint8_t reserved0 : 1;
	uint8_t interfaceSpeedSupport : 4;
	uint8_t commandListOverride : 1;
	uint8_t ledSupport : 1;
	uint8_t aggressivePowerMgmt : 1;
	uint8_t staggeredSpinUp : 1;
	uint8_t mechanicalPresenceSwitch : 1;
	uint8_t notificationSupport : 1;
	uint8_t nativeCommandQueuing : 1;
	uint8_t bit64Addressing : 1;
} __attribute__((packed));
typedef struct AHCIHostCapabilities AHCIHostCapabilities;

struct AHCIHostBusAdapter {
	AHCIHostCapabilities hostCapabilities;
	uint32_t globalHostControl;
	uint32_t interruptStatus;
	uint32_t portsImplemented;
	uint32_t version;
	uint32_t cccControl;
	uint32_t cccPorts;
	uint32_t enclosureMgmtLocation;
	uint32_t enclosureMgmtControl;
	uint32_t hostCapabilitiesExt;
	uint32_t biosHandOffControl;
	uint8_t reserved0[116];
	uint8_t vendor[96];
	AHCIPort ports[AHCI_PORT_COUNT];
} __attribute__((packed));
typedef struct AHCIHostBusAdapter AHCIHostBusAdapter;

struct AHCICommandHeader {
	uint8_t commandFisLength : 5;
	uint8_t atapi : 1;
	uint8_t write : 1;
	uint8_t prefetchable : 1;
	uint8_t reset : 1;
	uint8_t bist : 1;
	uint8_t clearBusy : 1;
	uint8_t reserved0 : 1;
	uint8_t portMultiplierPort : 4;
	uint16_t prdtLength;
	uint32_t prdtCount;
	uint32_t commandTableBase;
	uint32_t commandTableBaseUpper;
	uint32_t reserved1[4];
} __attribute__((packed));
typedef struct AHCICommandHeader AHCICommandHeader;

struct AHCIFISRegisterH2D {
	uint8_t fisType;
	uint8_t portMultiplierPort : 4;
	uint8_t reserved0 : 3;
	uint8_t commandControl : 1;
	uint8_t command;
	uint8_t featureLow;
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t featureHigh;
	uint8_t countLow;
	uint8_t countHigh;
	uint8_t isochronousCommandCompletion;
	uint8_t control;
	uint32_t reserved1;
} __attribute__((packed));
typedef struct AHCIFISRegisterH2D AHCIFISRegisterH2D;

struct AHCIPRDTEntry {
	uint32_t dataBase;
	uint32_t dataBaseUpper;
	uint32_t reserved0;
	uint32_t byteCount : 22;
	uint16_t reserved1 : 9;
	uint8_t interruptOnCompletion : 1;
} __attribute__((packed));
typedef struct AHCIPRDTEntry AHCIPRDTEntry;

struct AHCICommandTable {
	uint8_t commandFIS[64];
	uint8_t atapiCommand[16];
	uint8_t reserved[48];
	AHCIPRDTEntry prdtEntries[];
} __attribute__((packed));
typedef struct AHCICommandTable AHCICommandTable;

struct AHCIDevice {
	uint8_t type;
	uint8_t portNumber;
	volatile AHCIPort *port;
	AHCICommandHeader *commandHeaders;
	void *fisBase;
	AHCICommandTable *commandTables[AHCI_COMMAND_LIST_SIZE / sizeof(AHCICommandHeader)];
};
typedef struct AHCIDevice AHCIDevice;

struct AHCIController {
	volatile AHCIHostBusAdapter *hba;
	size_t deviceCount;
	AHCIDevice *devices[AHCI_PORT_COUNT];
	struct AHCIController *next;
};
typedef struct AHCIController AHCIController;

extern AHCIController *ahciControllers;

extern void ahciMsiHandler();
extern void ahciMsiHandlerWrapper();
extern bool ahciRead(AHCIController *controller, AHCIDevice *device, size_t startSector, size_t count, void *buffer);
extern bool configureAhciDevice(AHCIController *controller, AHCIDevice *device);
extern bool initializeAHCI(PCIeFunction *pcieFunction);
extern void startAhciCommand(AHCIDevice *device);
extern void stopAhciCommand(AHCIDevice *device);