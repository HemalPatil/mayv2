#pragma once

#include <drivers/filesystems.h>
#include <map>

namespace Drivers {
namespace FS {
	class JolietISO : public BaseFS {
		public:
			static const std::string cdSignature;
			static const uint8_t directoryFlag = 2;

			enum VolumeDescriptorType : uint8_t {
				Boot = 0,
				Primary,
				Supplementary,
				Terminator = 0xff
			};

			struct DirectoryRecord {
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
				char fileName[0];
			} __attribute__((packed));

			struct VolumeDescriptorHeader {
				VolumeDescriptorType type;
				char signature[5];
				uint8_t version;
			} __attribute__((packed));

			struct SupplementaryVolumeDescriptor {
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
				DirectoryRecord rootDirectory;
				// Add 1 byte padding because directory record length is variable
				uint8_t directoryRecordPadding;
				// TODO: add other attributes present in the Primary Volume Descriptor
				uint8_t reserved3[1858];
			} __attribute__((packed));

		private:
			size_t lbaSize;
			size_t rootDirLba;
			size_t rootDirExtentSize;
			std::map<std::string, std::vector<DirectoryEntry>> cachedDirectoryEntries;

			static std::vector<DirectoryEntry> extentToEntries(
				const DirectoryRecord* const extent,
				size_t lba,
				size_t size,
				size_t parentLba,
				size_t parentSize
			);

		public:
			JolietISO(std::shared_ptr<Storage::BlockDevice> device, const Storage::Buffer &svdSectorBuffer);

			Async::Thenable<bool> initialize();

			// Reads directory at given absolute path ending in '/'
			Async::Thenable<ReadDirectoryResult> readDirectory(const std::string &absolutePath) override;

			// TODO: should accept a file descriptor instead that was obtained by opening the file first
			Async::Thenable<ReadFileResult> readFile(const std::string &absolutePath) override;

			static Async::Thenable<Storage::Buffer> isJolietIso(std::shared_ptr<Storage::BlockDevice> device);
	};
}
}
