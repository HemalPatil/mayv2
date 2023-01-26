#pragma once

#include <async.h>
#include <pcie.h>

#define ATAPI_READTOC 0xa8

#define AHCI_COMMAND_LIST_SIZE 1024
#define AHCI_COMMAND_READ_DMA_EX 0x25
#define AHCI_COMMAND_ATAPI_PACKET 0xa0

#define AHCI_DEVICE_BUSY 0x80
#define AHCI_DEVICE_DRQ 0x08

#define AHCI_FIS_TYPE_REG_D2H 0x34
#define AHCI_FIS_TYPE_REG_H2D 0x27

#define AHCI_IDENTIFY_DEVICE 0xec
#define AHCI_IDENTIFY_PACKET_DEVICE 0xa1

#define AHCI_PORT_ACTIVE 1
#define AHCI_PORT_COUNT 32
#define AHCI_PORT_DEVICE_PRESENT 3

#define AHCI_SECTOR_SIZE 512

#define AHCI_MODE ((uint32_t)1 << 31)
#define AHCI_GHC_INTERRUPT_ENABLE ((uint32_t)1 << 1)
#define AHCI_TASK_FILE_ERROR ((uint32_t)1 << 30)
#define AHCI_REGISTER_D2H 1

namespace AHCI {
	// Refer AHCI spec https://www.intel.com.au/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
	struct PortStatus {
		uint8_t deviceDetection : 4;
		uint8_t interfaceSpeed : 4;
		uint8_t powerMgmt : 4;
		uint8_t reserved0 : 4;
		uint16_t reserved1;
	} __attribute__((packed));

	struct PortCommandStatus {
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

	struct Port {
		uint32_t commandListBase;
		uint32_t commandListBaseUpper;
		uint32_t fisBase;
		uint32_t fisBaseUpper;
		uint32_t interruptStatus;
		uint32_t interruptEnable;
		PortCommandStatus commandStatus;
		uint32_t reserved0;
		uint32_t taskFileData;
		uint32_t signature;
		PortStatus sataStatus;
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

	struct HostCapabilities {
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

	struct HostBusAdapter {
		HostCapabilities hostCapabilities;
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
		Port ports[AHCI_PORT_COUNT];
	} __attribute__((packed));

	struct CommandHeader {
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

	namespace FIS {

		struct RegisterH2D {
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

	}

	struct PRDTEntry {
		uint32_t dataBase;
		uint32_t dataBaseUpper;
		uint32_t reserved0;
		uint32_t byteCount : 22;
		uint16_t reserved1 : 9;
		uint8_t interruptOnCompletion : 1;
	} __attribute__((packed));

	struct CommandTable {
		uint8_t commandFIS[64];
		uint8_t atapiCommand[16];
		uint8_t reserved[48];
		PRDTEntry prdtEntries[];
	} __attribute__((packed));

	struct IdentifyDeviceData {
		struct {
			uint16_t reserved1 : 1;
			uint16_t retired3 : 1;
			uint16_t responseIncomplete : 1;
			uint16_t retired2 : 3;
			uint16_t fixedDevice : 1;
			uint16_t removableMedia : 1;
			uint16_t retired1 : 7;
			uint16_t deviceType : 1;
		} generalConfiguration;
		uint16_t cylinderCount;
		uint16_t specificConfiguration;
		uint16_t headCount;
		uint16_t retired1[2];
		uint16_t sectorsPerTrack;
		uint16_t vendorUnique1[3];
		uint8_t serialNumber[20];
		uint16_t retired2[2];
		uint16_t obsolete1;
		uint8_t firmwareRevision[8];
		uint8_t modelNumber[40];
		uint8_t maximumBlockTransfer;
		uint8_t vendorUnique2;
		struct {
			uint16_t featureSupported : 1;
			uint16_t reserved : 15;
		} trustedComputing;
		struct {
			uint8_t currentLongPhysicalSectorAlignment : 2;
			uint8_t reservedByte49 : 6;
			uint8_t dmaSupported : 1;
			uint8_t lbaSupported : 1;
			uint8_t ioRdyDisable : 1;
			uint8_t ioRdySupported : 1;
			uint8_t reserved1 : 1;
			uint8_t standybyTimerSupport : 1;
			uint8_t reserved2 : 2;
			uint16_t reservedWord50;
		} capabilities;
		uint16_t obsoleteWords51[2];
		uint16_t translationFieldsValid : 3;
		uint16_t reserved3 : 5;
		uint16_t freeFallControlSensitivity : 8;
		uint16_t currentCylinderCount;
		uint16_t currentHeadCount;
		uint16_t currentSectorsPerTrack;
		uint32_t currentSectorCapacity;
		uint8_t currentMultiSectorSetting;
		uint8_t multiSectorSettingValid : 1;
		uint8_t reservedByte59 : 3;
		uint8_t sanitizeFeatureSupported : 1;
		uint8_t cryptoScrambleExtCommandSupported : 1;
		uint8_t overwriteExtCommandSupported : 1;
		uint8_t blockEraseExtCommandSupported : 1;
		uint32_t userAddressableSectors;
		uint16_t obsoleteWord62;
		uint16_t multiWordDMASupport : 8;
		uint16_t multiWordDMAActive : 8;
		uint16_t advancedPIOModes : 8;
		uint16_t reservedByte64 : 8;
		uint16_t minimumMWXferCycleTime;
		uint16_t recommendedMWXferCycleTime;
		uint16_t minimumPIOCycleTime;
		uint16_t minimumPIOCycleTimeIORDY;
		struct {
			uint16_t zonedCapabilities : 2;
			uint16_t nonVolatileWriteCache : 1;
			uint16_t extendedUserAddressableSectorsSupported : 1;
			uint16_t deviceEncryptsAllUserData : 1;
			uint16_t readZeroAfterTrimSupported : 1;
			uint16_t optional28BitCommandsSupported : 1;
			uint16_t ieee1667 : 1;
			uint16_t downloadMicrocodeDmaSupported : 1;
			uint16_t setMaxSetPasswordUnlockDmaSupported : 1;
			uint16_t writeBufferDmaSupported : 1;
			uint16_t readBufferDmaSupported : 1;
			uint16_t deviceConfigIdentifySetDmaSupported : 1;
			uint16_t LPSAERCSupported : 1;
			uint16_t deterministicReadAfterTrimSupported : 1;
			uint16_t cFastSpecSupported : 1;
		} additionalSupported;
		uint16_t reservedWords70[5];
		uint16_t queueDepth : 5;
		uint16_t reservedWord75 : 11;
		struct {
			uint16_t reserved0 : 1;
			uint16_t sataGen1 : 1;
			uint16_t sataGen2 : 1;
			uint16_t sataGen3 : 1;
			uint16_t reserved1 : 4;
			uint16_t nCQ : 1;
			uint16_t hipm : 1;
			uint16_t phyEvents : 1;
			uint16_t ncqUnload : 1;
			uint16_t ncqPriority : 1;
			uint16_t hostAutoPS : 1;
			uint16_t deviceAutoPS : 1;
			uint16_t readLogDMA : 1;
			uint16_t reserved2 : 1;
			uint16_t currentSpeed : 3;
			uint16_t ncqStreaming : 1;
			uint16_t ncqQueueMgmt : 1;
			uint16_t ncqReceiveSend : 1;
			uint16_t DEVSLPtoReducedPwrState : 1;
			uint16_t reserved3 : 8;
		} sataCapabilities;
		struct {
			uint16_t reserved0 : 1;
			uint16_t nonZeroOffsets : 1;
			uint16_t dmaSetupAutoActivate : 1;
			uint16_t dipm : 1;
			uint16_t inOrderData : 1;
			uint16_t hardwareFeatureControl : 1;
			uint16_t softwareSettingsPreservation : 1;
			uint16_t ncqAutosense : 1;
			uint16_t devSlp : 1;
			uint16_t hybridInformation : 1;
			uint16_t reserved1 : 6;
		} sataFeaturesSupported;
		struct {
			uint16_t reserved0 : 1;
			uint16_t nonZeroOffsets : 1;
			uint16_t dmaSetupAutoActivate : 1;
			uint16_t dipm : 1;
			uint16_t inOrderData : 1;
			uint16_t hardwareFeatureControl : 1;
			uint16_t softwareSettingsPreservation : 1;
			uint16_t deviceAutoPS : 1;
			uint16_t devslp : 1;
			uint16_t hybridInformation : 1;
			uint16_t reserved1 : 6;
		} sataFeaturesEnabled;
		uint16_t majorRevision;
		uint16_t minorRevision;
		struct {
			uint16_t smartCommands : 1;
			uint16_t securityMode : 1;
			uint16_t removableMediaFeature : 1;
			uint16_t powerManagement : 1;
			uint16_t reserved1 : 1;
			uint16_t writeCache : 1;
			uint16_t lookAhead : 1;
			uint16_t releaseInterrupt : 1;
			uint16_t serviceInterrupt : 1;
			uint16_t deviceReset : 1;
			uint16_t hostProtectedArea : 1;
			uint16_t obsolete1 : 1;
			uint16_t writeBuffer : 1;
			uint16_t readBuffer : 1;
			uint16_t nop : 1;
			uint16_t obsolete2 : 1;
			uint16_t downloadMicrocode : 1;
			uint16_t dmaQueued : 1;
			uint16_t cfa : 1;
			uint16_t advancedPm : 1;
			uint16_t msn : 1;
			uint16_t powerUpInStandby : 1;
			uint16_t manualPowerUp : 1;
			uint16_t reserved2 : 1;
			uint16_t setMax : 1;
			uint16_t acoustics : 1;
			uint16_t bigLba : 1;
			uint16_t deviceConfigOverlay : 1;
			uint16_t flushCache : 1;
			uint16_t flushCacheExt : 1;
			uint16_t wordValid83 : 2;
			uint16_t smartErrorLog : 1;
			uint16_t smartSelfTest : 1;
			uint16_t mediaSerialNumber : 1;
			uint16_t mediaCardPassThrough : 1;
			uint16_t streamingFeature : 1;
			uint16_t gpLogging : 1;
			uint16_t writeFua : 1;
			uint16_t writeQueuedFua : 1;
			uint16_t wwn64Bit : 1;
			uint16_t urgReadStream : 1;
			uint16_t urgWriteStream : 1;
			uint16_t reservedForTechReport : 2;
			uint16_t idleWithUnloadFeature : 1;
			uint16_t wordValid : 2;
		} commandSetSupport;
		struct {
			uint16_t smartCommands : 1;
			uint16_t securityMode : 1;
			uint16_t removableMediaFeature : 1;
			uint16_t powerManagement : 1;
			uint16_t reserved1 : 1;
			uint16_t writeCache : 1;
			uint16_t lookAhead : 1;
			uint16_t releaseInterrupt : 1;
			uint16_t serviceInterrupt : 1;
			uint16_t deviceReset : 1;
			uint16_t hostProtectedArea : 1;
			uint16_t obsolete1 : 1;
			uint16_t writeBuffer : 1;
			uint16_t readBuffer : 1;
			uint16_t nop : 1;
			uint16_t obsolete2 : 1;
			uint16_t downloadMicrocode : 1;
			uint16_t dmaQueued : 1;
			uint16_t cfa : 1;
			uint16_t advancedPm : 1;
			uint16_t msn : 1;
			uint16_t powerUpInStandby : 1;
			uint16_t manualPowerUp : 1;
			uint16_t reserved2 : 1;
			uint16_t setMax : 1;
			uint16_t acoustics : 1;
			uint16_t bigLba : 1;
			uint16_t deviceConfigOverlay : 1;
			uint16_t flushCache : 1;
			uint16_t flushCacheExt : 1;
			uint16_t resrved3 : 1;
			uint16_t words119_120Valid : 1;
			uint16_t smartErrorLog : 1;
			uint16_t smartSelfTest : 1;
			uint16_t mediaSerialNumber : 1;
			uint16_t mediaCardPassThrough : 1;
			uint16_t streamingFeature : 1;
			uint16_t gpLogging : 1;
			uint16_t writeFua : 1;
			uint16_t writeQueuedFua : 1;
			uint16_t wwn64Bit : 1;
			uint16_t urgReadStream : 1;
			uint16_t urgWriteStream : 1;
			uint16_t reservedForTechReport : 2;
			uint16_t idleWithUnloadFeature : 1;
			uint16_t reserved4 : 2;
		} commandSetActive;
		uint16_t ultraDMASupport : 8;
		uint16_t ultraDMAActive : 8;
		struct {
			uint16_t timeRequired : 15;
			uint16_t extendedTimeReported : 1;
		} normalSecurityEraseUnit;
		struct {
			uint16_t timeRequired : 15;
			uint16_t extendedTimeReported : 1;
		} enhancedSecurityEraseUnit;
		uint16_t currentAPMLevel : 8;
		uint16_t reservedWord91 : 8;
		uint16_t masterPasswordID;
		uint16_t hardwareResetResult;
		uint16_t currentAcousticValue : 8;
		uint16_t recommendedAcousticValue : 8;
		uint16_t streamMinRequestSize;
		uint16_t streamingTransferTimeDMA;
		uint16_t streamingAccessLatencyDMAPIO;
		uint32_t streamingPerfGranularity;
		uint64_t max48BitLBA;
		uint16_t streamingTransferTime;
		uint16_t dsmCap;
		struct {
			uint16_t logicalSectorsPerPhysicalSector : 4;
			uint16_t reserved0 : 8;
			uint16_t logicalSectorLongerThan256Words : 1;
			uint16_t multipleLogicalSectorsPerPhysicalSector : 1;
			uint16_t valid : 2;
		} physicalLogicalSectorSize;
		uint16_t interSeekDelay;
		uint16_t worldWideName[4];
		uint16_t reservedForWorldWideName128[4];
		uint16_t reservedForTlcTechnicalReport;
		uint32_t wordsPerLogicalSector;
		struct {
			uint16_t reservedForDrqTechnicalReport : 1;
			uint16_t writeReadVerify : 1;
			uint16_t writeUncorrectableExt : 1;
			uint16_t readWriteLogDmaExt : 1;
			uint16_t downloadMicrocodeMode3 : 1;
			uint16_t freefallControl : 1;
			uint16_t senseDataReporting : 1;
			uint16_t extendedPowerConditions : 1;
			uint16_t reserved0 : 6;
			uint16_t wordValid : 2;
		} commandSetSupportExt;
		struct {
			uint16_t reservedForDrqTechnicalReport : 1;
			uint16_t writeReadVerify : 1;
			uint16_t writeUncorrectableExt : 1;
			uint16_t readWriteLogDmaExt : 1;
			uint16_t downloadMicrocodeMode3 : 1;
			uint16_t freefallControl : 1;
			uint16_t senseDataReporting : 1;
			uint16_t extendedPowerConditions : 1;
			uint16_t reserved0 : 6;
			uint16_t reserved1 : 2;
		} commandSetActiveExt;
		uint16_t reservedForExpandedSupportandActive[6];
		uint16_t msnSupport : 2;
		uint16_t reservedWord127 : 14;
		struct {
			uint16_t securitySupported : 1;
			uint16_t securityEnabled : 1;
			uint16_t securityLocked : 1;
			uint16_t securityFrozen : 1;
			uint16_t securityCountExpired : 1;
			uint16_t enhancedSecurityEraseSupported : 1;
			uint16_t reserved0 : 2;
			uint16_t securityLevel : 1;
			uint16_t reserved1 : 7;
		} securityStatus;
		uint16_t reservedWord129[31];
		struct {
			uint16_t maximumCurrentInMA : 12;
			uint16_t cfaPowerMode1Disabled : 1;
			uint16_t cfaPowerMode1Required : 1;
			uint16_t reserved0 : 1;
			uint16_t word160Supported : 1;
		} cfaPowerMode1;
		uint16_t reservedForCfaWord161[7];
		uint16_t nominalFormFactor : 4;
		uint16_t reservedWord168 : 12;
		struct {
			uint16_t supportsTrim : 1;
			uint16_t reserved0 : 15;
		} dataSetManagementFeature;
		uint16_t additionalProductID[4];
		uint16_t reservedForCfaWord174[2];
		uint16_t currentMediaSerialNumber[30];
		struct {
			uint16_t supported : 1;
			uint16_t reserved0 : 1;
			uint16_t writeSameSuported : 1;
			uint16_t errorRecoveryControlSupported : 1;
			uint16_t featureControlSuported : 1;
			uint16_t dataTablesSuported : 1;
			uint16_t reserved1 : 6;
			uint16_t vendorSpecific : 4;
		} sctCommandTransport;
		uint16_t reservedWord207[2];
		struct {
			uint16_t alignmentOfLogicalWithinPhysical : 14;
			uint16_t word209Supported : 1;
			uint16_t reserved0 : 1;
		} blockAlignment;
		uint16_t writeReadVerifySectorCountMode3Only[2];
		uint16_t writeReadVerifySectorCountMode2Only[2];
		struct {
			uint16_t nvCachePowerModeEnabled : 1;
			uint16_t reserved0 : 3;
			uint16_t nvCacheFeatureSetEnabled : 1;
			uint16_t reserved1 : 3;
			uint16_t nvCachePowerModeVersion : 4;
			uint16_t nvCacheFeatureSetVersion : 4;
		} nvCacheCapabilities;
		uint16_t nvCacheSizeLSW;
		uint16_t nvCacheSizeMSW;
		uint16_t nominalMediaRotationRate;
		uint16_t reservedWord218;
		struct {
			uint8_t nvCacheEstimatedTimeToSpinUpInSeconds;
			uint8_t reserved;
		} nvCacheOptions;
		uint16_t writeReadVerifySectorCountMode : 8;
		uint16_t reservedWord220 : 8;
		uint16_t reservedWord221;
		struct {
			uint16_t majorVersion : 12;
			uint16_t transportType : 4;
		} transportMajorVersion;
		uint16_t transportMinorVersion;
		uint16_t reservedWord224[6];
		uint32_t extendedNumberOfUserAddressableSectors[2];
		uint16_t minBlocksPerDownloadMicrocodeMode03;
		uint16_t maxBlocksPerDownloadMicrocodeMode03;
		uint16_t reservedWord236[19];
		uint16_t signature : 8;
		uint16_t checkSum : 8;
	} __attribute__((packed));

	class Controller;
	class Device;
	class SataDevice;
	class SatapiDevice;

	enum PortSignature : uint32_t {
		SATA = 0x00000101,
		SATAPI = 0xeb140101
	};

	extern std::vector<Controller> controllers;

	extern Async::Thenable<bool> initialize(PCIe::Function &pcieFunction);
}

extern "C" void ahciMsiHandler();
extern "C" void ahciMsiHandlerWrapper();
