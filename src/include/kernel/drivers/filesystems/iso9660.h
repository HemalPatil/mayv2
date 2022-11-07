#pragma once

#include <drivers/filesystems.h>

namespace FS {
	class ISO9660 : public BaseFS {
		public:
			static constexpr const char* const cdSignature = "CD001";

			enum VolumeDescriptorType : uint8_t {
				Boot = 0,
				Primary,
				Terminator = 0xff
			};

			struct Directory {
				uint8_t length;
				uint8_t extendedAttributeLength;
				uint32_t extentLba;
				uint32_t extentLbaMsb;
				uint32_t extentSize;
				uint32_t extentSizeMsb;
				char dateTime[7];
				uint8_t flags;
				uint8_t interleavedFileUnitSize;
				uint8_t interleavedGapSize;
				uint16_t volumeSequenceNumber;
				uint16_t volumeSequenceNumberMsb;
				uint8_t fileNameLength;
				char fileName;
			} __attribute__((packed));

			struct VolumeDescriptorHeader {
				VolumeDescriptorType type;
				char signature[5];
				uint8_t version;
			} __attribute__((packed));

			struct PrimaryVolumeDescriptor {
				VolumeDescriptorHeader header;
				uint8_t reserved0;
				char systemId[32];
				char volumeId[32];
				uint64_t reserved1;
				uint32_t volumeSpaceSize;
				uint32_t volumeSpaceSizeMsb;
				uint8_t reserved2[32];
				uint16_t volumeSetSize;
				uint16_t volumeSetSizeMsb;
				uint16_t volumeSequenceNumber;
				uint16_t volumeSequenceNumberMsb;
				uint16_t lbaSize;
				uint16_t lbaSizeMsb;
				uint32_t pathTableSize;
				uint32_t pathTableSizeMsb;
				uint32_t lsbPathTableLba;
				uint32_t lsbOptionalPathTableLba;
				uint32_t msbPathTableLba;
				uint32_t msbOptionalPathTableLba;
				Directory rootDirectory;
				// TODO: add other attributes present in the Primary Volume Descriptor
				uint8_t reserved3[1858];
			} __attribute__((packed));

			ISO9660(Storage::BlockDevice &device) : BaseFS(device) {}
			bool openFile() override;

			static bool isIso9660(Storage::BlockDevice *device);
	};
}
