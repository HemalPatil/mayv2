#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#define BOOTLOADER_PADDING 32
#define DAP_SIZE 16
#define ELFPARSE_ORIGIN 0x00020000
#define FILE_NAME_OFFSET 33
#define FILE_NAME_SIZE_OFFSET 32
#define ISO_SECTOR_SIZE 2048
#define L32K64_SCRATCH_BASE 0x80000
#define RECORD_SIZE_OFFSET 10
#define ROOT_DIRECTORY_OFFSET 156
#define SECTOR_LOCATION_OFFSET 2

const int moduleCount = 9; // Number of modules that are required by the kernel to bootstrap

bool findRecord(const char *directory, const int dirSize, const std::u16string &recordName, int &recordSector, int &recordSize) {
	bool recordFound = false;
	// Skip first 0x44 bytes, they are directory records for current and parent directory
	int dirSeekg = 0x44;
	// Check if dirSeekg is past the directory extent or all records have been traversed
	while ((dirSeekg < dirSize) && (*((char *)(directory + dirSeekg)) > 0)) {
		int dirRecordSize = *((char *)(directory + dirSeekg)); // Get the size of the record in the directory extent
		int fileNameSize = *((char *)(directory + dirSeekg + FILE_NAME_SIZE_OFFSET)); // Get length of the file name
		char *fileName = new char[fileNameSize];
		memcpy(fileName, directory + dirSeekg + FILE_NAME_OFFSET, fileNameSize);
		// Flip bytes of UTF16BE record name
		for (int i = 0; i < fileNameSize; i += 2) {
			char temp = fileName[i];
			fileName[i] = fileName[i + 1];
			fileName[i + 1] = temp;
		}
		std::u16string u16FileName = std::u16string((char16_t*)fileName, (char16_t*)(fileName + fileNameSize));
		if (u16FileName == recordName) {
			// Return parameters like file size and sector
			recordSector = *((int *)(directory + dirSeekg + SECTOR_LOCATION_OFFSET));
			recordSize = *((int *)(directory + dirSeekg + RECORD_SIZE_OFFSET));
			recordFound = true;
			break;
		}
		delete[] fileName;
		dirSeekg += dirRecordSize; // Move to next record in the directory extent
	}
	return recordFound;
}

int main(int argc, char *argv[]) {
	// Check if exactly 3 arguments i.e. EXE name, ISO file name, and root FS GUID are provided
	if (argc != 3) {
		std::cerr << "Supply only the disk ISO name and root FS GUID" << std::endl;
		return 1;
	}
	char *isoName = argv[1];
	std::fstream isoFile;
	isoFile.open(isoName, std::ios::in | std::ios::out | std::ios::binary);
	if (!isoFile.is_open()) {
		std::cerr << "Could not open ISO " << isoName << std::endl;
		isoFile.close();
		return 1;
	}

	// Skip first 32KiB (i.e. 16 sectors of ISO always empty according to ISO-9660)
	size_t svdAddress = 16 * ISO_SECTOR_SIZE;
	char *sector = new char[ISO_SECTOR_SIZE];
	memset(sector, 0, ISO_SECTOR_SIZE);
	char svdSignature[] = { 0x02, 'C', 'D', '0', '0', '1', 0x01, 0x00 };
	bool svdFound = false;
	int rootDirectorySize = 0, rootDirectorySector = 0;

	// Find supplementary volume descriptor
	while (!svdFound && !isoFile.eof()) {
		isoFile.seekg(svdAddress, std::ios::beg);
		isoFile.read(sector, ISO_SECTOR_SIZE);
		if (std::equal(svdSignature, svdSignature + 7, sector)) {
			// Get root directory extent sector and size
			rootDirectorySector = *((int *)(sector + ROOT_DIRECTORY_OFFSET + SECTOR_LOCATION_OFFSET));
			rootDirectorySize = *((int *)(sector + ROOT_DIRECTORY_OFFSET + RECORD_SIZE_OFFSET));
			svdFound = true;
		} else {
			svdAddress += ISO_SECTOR_SIZE; // If not found, move on to next sector in the ISO file
		}
	}
	delete[] sector;
	if (!svdFound) {
		std::cerr << "Not a valid Joliet ISO file" << std::endl;
		isoFile.close();
		return 1;
	}

	char *rootDirectory = new char[rootDirectorySize];
	isoFile.seekg(rootDirectorySector * ISO_SECTOR_SIZE, std::ios::beg);
	isoFile.read(rootDirectory, rootDirectorySize);
	int bootDirSector, bootDirSize;
	if (!findRecord(rootDirectory, rootDirectorySize, u"boot", bootDirSector, bootDirSize)) {
		delete[] rootDirectory;
		isoFile.close();
		std::cerr << "[boot] directory not found!" << std::endl;
		return 1;
	}
	delete[] rootDirectory;

	char *bootDir = new char[bootDirSize];
	isoFile.seekg(bootDirSector * ISO_SECTOR_SIZE, std::ios::beg);
	isoFile.read(bootDir, bootDirSize);
	int stage1DirSector, stage1DirSize, stage2DirSector, stage2DirSize, kernel64Sector, kernel64Size;
	if (!findRecord(bootDir, bootDirSize, u"stage1", stage1DirSector, stage1DirSize)) {
		delete[] bootDir;
		isoFile.close();
		std::cerr << "[stage1] directory not found!" << std::endl;
		return 1;
	}
	if (!findRecord(bootDir, bootDirSize, u"stage2", stage2DirSector, stage2DirSize)) {
		delete[] bootDir;
		isoFile.close();
		std::cerr << "[stage2] directory not found!" << std::endl;
		return 1;
	}
	if (!findRecord(bootDir, bootDirSize, u"kernel", kernel64Sector, kernel64Size)) {
		delete[] bootDir;
		isoFile.close();
		std::cerr << "[kernel] not found!" << std::endl;
		return 1;
	}
	delete[] bootDir;

	char *stage1Dir = new char[stage1DirSize];
	isoFile.seekg(stage1DirSector * ISO_SECTOR_SIZE, std::ios::beg);
	isoFile.read(stage1Dir, stage1DirSize);
	int bootBinSector, bootBinSize;
	if (!findRecord(stage1Dir, stage1DirSize, u"bootload.bin", bootBinSector, bootBinSize)) {
		delete[] stage1Dir;
		isoFile.close();
		std::cerr << "[bootload.bin] not found!" << std::endl;
		return 1;
	}
	char *stage2Dir = new char[stage2DirSize];
	isoFile.seekg(stage2DirSector * ISO_SECTOR_SIZE, std::ios::beg);
	isoFile.read(stage2Dir, stage2DirSize);
	std::u16string coreFiles[] = {
		u"x64.bin",
		u"mmap.bin",
		u"videoModes.bin",
		u"a20.bin",
		u"gdt.bin",
		u"k64load.bin",
		u"elfParse.bin",
		u"loader32",
		u"kernel"
	};
	int coreFileSectors[moduleCount], coreFileSizes[moduleCount], coreFileSegments[moduleCount];
	for (int i = 0; i < 5; ++i) {
		if (!findRecord(stage1Dir, stage1DirSize, coreFiles[i], coreFileSectors[i], coreFileSizes[i])) {
			delete[] stage1Dir;
			isoFile.close();
			std::cerr << "coreFiles[" << i << "] not found!" << std::endl;
			return 1;
		}
	}
	for (int i = 5; i < 8; ++i) {
		if (!findRecord(stage2Dir, stage2DirSize, coreFiles[i], coreFileSectors[i], coreFileSizes[i])) {
			delete[] stage2Dir;
			isoFile.close();
			std::cerr << "coreFiles[" << i << "] not found!" << std::endl;
			return 1;
		}
	}
	delete[] stage1Dir;
	delete[] stage2Dir;

	coreFileSegments[0] = 0x0080;
	coreFileSectors[8] = kernel64Sector;
	coreFileSizes[8] = kernel64Size;
	for (int i = 1; i < moduleCount; ++i) {
		coreFileSegments[i] = coreFileSegments[i - 1] + ceil((double)coreFileSizes[i - 1] / 16);
	}
	int bootBinSeekp = bootBinSector * ISO_SECTOR_SIZE + BOOTLOADER_PADDING; // Skip first 32 bytes (contains int 0x22 routine)
	for (int i = 0; i < moduleCount; ++i) {
		isoFile.seekp(bootBinSeekp + 2, std::ios::beg); // Skip first 2 bytes of each DAP
		uint16_t sectorCount = ceil((double)coreFileSizes[i] / ISO_SECTOR_SIZE); // DAP number of sectors to load
		isoFile.write((char *)&sectorCount, 2);
		uint16_t dapOffset = 0x0000; // Offset at which data is loaded is always 0x00
		isoFile.write((char *)&dapOffset, 2);
		if (6 == i) {
			// elfParse.bin
			// FIXME: assumes memory from ELFPARSE_ORIGIN will be free to load elfParse.bin
			uint16_t elfParseSegment = ELFPARSE_ORIGIN >> 4;
			isoFile.write((char *)&elfParseSegment, 2);
		} else if (7 == i || 8 == i) {
			// loader32 or kernel
			// Memory from L32K64_SCRATCH_BASE to L32K64_SCRATCH_BASE + L32K64_SCRATCH_LENGTH is assumed to be free
			// mmap.asm will assert this during boot
			uint16_t loader32ElfSegment = L32K64_SCRATCH_BASE >> 4;
			isoFile.write((char *)&loader32ElfSegment, 2);
		} else {
			isoFile.write((char *)&(coreFileSegments[i]), 2); // Segment at which data is loaded
		}
		isoFile.write((char *)&(coreFileSectors[i]), 4); // First sector of the file to be loaded
		bootBinSeekp += 16;
	}

	// Copy the root FS GUID
	isoFile.seekp(bootBinSector * ISO_SECTOR_SIZE + BOOTLOADER_PADDING + moduleCount * 16);
	isoFile.write(argv[2], 36);

	std::cout
		<< "ISO formatted (Kernel startSector[" << kernel64Sector
		<< "] sectorCount[" << ceil((double)coreFileSizes[8] / ISO_SECTOR_SIZE) << "])\n";

	isoFile.close();
	return 0;
}
