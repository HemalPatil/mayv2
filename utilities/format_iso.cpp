#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <stdint.h>
using namespace std;

#define ISO_SECTOR_SIZE 0x800

const int moduleCount = 9; // Number of modules that are required by the kernel to bootstrap

bool findRecord(const char *directory, const int dirSize, const char *recordName, int &recordSector, int &recordSize) {
	bool recordFound = false;
	// Skip first 0x44 bytes, they are directory records for current and parent directory
	int dirSeekg = 0x44;
	// Check if we are past the directory extent or all records have been traversed
	while ((dirSeekg < dirSize) && (*((char *)(directory + dirSeekg)) > 0)) {
		int dirRecordSize = *((char *)(directory + dirSeekg)); // Get the size of the record in the directory extent
		int fileNameSize = *((char *)(directory + dirSeekg + 32)); // Get length of the file name
		char *fileName = new char[fileNameSize + 1]; // Copy the record name in a buffer
		memcpy(fileName, directory + dirSeekg + 33, fileNameSize);
		fileName[fileNameSize] = 0;
		if (strcmp(fileName, recordName) == 0) {
			// Return parameters like file size and sector
			recordSector = *((int *)(directory + dirSeekg + 2));
			recordSize = *((int *)(directory + dirSeekg + 10));
			recordFound = true;
			break;
		}
		delete[] fileName;
		dirSeekg += dirRecordSize; // Move to next record in the directory extent
	}
	return recordFound;
}

int main(int argc, char *argv[]) {
	// Check if exactly 2 arguments i.e. EXE name and ISO file name are provided
	if (argc != 2) {
		cerr << "Supply only the disk ISO name" << endl;
		return 1;
	}
	char *isoName = new char[strlen(argv[1])];
	strcpy(isoName, argv[1]);
	fstream isoFile;
	isoFile.open(isoName, ios::in | ios::out | ios::binary);
	if (!isoFile.is_open()) {
		cerr << "Could not open ISO " << isoName << endl;
		isoFile.close();
		return 1;
	}

	// Skip first 32KiB (i.e. 16 sectors of ISO always empty according to ISO-9660)
	int volumeDescriptorAddress = 16 * ISO_SECTOR_SIZE;
	char *sector = new char[ISO_SECTOR_SIZE];
	memset(sector, 0, ISO_SECTOR_SIZE);
	char primaryVolumeDescriptor[] = { 0x01, 'C', 'D', '0', '0', '1', 0x01, 0x00 };
	bool volumeDescriptorFound = false;
	int rootDirectorySize = 0, rootDirectorySector = 0;
	while (!volumeDescriptorFound && !isoFile.eof()) {
		isoFile.seekg(volumeDescriptorAddress, ios::beg);
		isoFile.read(sector, ISO_SECTOR_SIZE);
		if (equal(primaryVolumeDescriptor, primaryVolumeDescriptor + 7, sector)) {
			rootDirectorySector = *((int *)(sector + 158)); // Get root directory extent sector and size
			rootDirectorySize = *((int *)(sector + 166));
			volumeDescriptorFound = true;
		} else {
			volumeDescriptorAddress += ISO_SECTOR_SIZE; // If not found, move on to next sector in the ISO file
		}
	}
	delete[] sector;
	if (!volumeDescriptorFound) {
		cerr << "Not a valid ISO file" << endl;
		isoFile.close();
		return 1;
	}

	char *rootDirectory = new char[rootDirectorySize];
	isoFile.seekg(rootDirectorySector * ISO_SECTOR_SIZE, ios::beg);
	isoFile.read(rootDirectory, rootDirectorySize);
	int bootDirSector, bootDirSize;
	if (!findRecord(rootDirectory, rootDirectorySize, "BOOT", bootDirSector, bootDirSize)) {
		delete[] rootDirectory;
		isoFile.close();
		cerr << "BOOT directory not found!" << endl;
		return 1;
	}
	delete[] rootDirectory;

	char *bootDir = new char[bootDirSize];
	isoFile.seekg(bootDirSector * ISO_SECTOR_SIZE, ios::beg);
	isoFile.read(bootDir, bootDirSize);
	int stage1DirSector, stage1DirSize, stage2DirSector, stage2DirSize, kernel64Sector, kernel64Size;
	if (!findRecord(bootDir, bootDirSize, "STAGE1", stage1DirSector, stage1DirSize)) {
		delete[] bootDir;
		isoFile.close();
		cerr << "STAGE1 directory not found!" << endl;
		return 1;
	}
	if (!findRecord(bootDir, bootDirSize, "STAGE2", stage2DirSector, stage2DirSize)) {
		delete[] bootDir;
		isoFile.close();
		cerr << "STAGE2 directory not found!" << endl;
		return 1;
	}
	if (!findRecord(bootDir, bootDirSize, "KERNEL.;1", kernel64Sector, kernel64Size)) {
		delete[] bootDir;
		isoFile.close();
		cerr << "KERNEL not found!" << endl;
		return 1;
	}
	delete[] bootDir;

	char *stage1Dir = new char[stage1DirSize];
	isoFile.seekg(stage1DirSector * ISO_SECTOR_SIZE, ios::beg);
	isoFile.read(stage1Dir, stage1DirSize);
	int bootBinSector, bootBinSize;
	if (!findRecord(stage1Dir, stage1DirSize, "BOOTLOAD.BIN;1", bootBinSector, bootBinSize)) {
		delete[] stage1Dir;
		isoFile.close();
		cerr << "BOOTLOAD.BIN file not found!" << endl;
		return 1;
	}
	char *stage2Dir = new char[stage2DirSize];
	isoFile.seekg(stage2DirSector * ISO_SECTOR_SIZE, ios::beg);
	isoFile.read(stage2Dir, stage2DirSize);
	char coreFiles[][15] = {
		"X64.BIN;1",
		"MMAP.BIN;1",
		"VIDMODES.BIN;1",
		"A20.BIN;1",
		"GDT.BIN;1",
		"K64LOAD.BIN;1",
		"ELFPARSE.BIN;1",
		"LOADER32.;1",
		"KERNEL.;1"
	};
	int coreFileSectors[moduleCount], coreFileSizes[moduleCount], coreFileSegments[moduleCount];
	for (int i = 0; i < 5; i++) {
		if (!findRecord(stage1Dir, stage1DirSize, coreFiles[i], coreFileSectors[i], coreFileSizes[i])) {
			delete[] stage1Dir;
			isoFile.close();
			cerr << coreFiles[i] << " not found!" << endl;
			return 1;
		}
	}
	for (int i = 5; i < 8; i++) {
		if (!findRecord(stage2Dir, stage2DirSize, coreFiles[i], coreFileSectors[i], coreFileSizes[i])) {
			delete[] stage2Dir;
			isoFile.close();
			cerr << coreFiles[i] << " not found!" << endl;
			return 1;
		}
	}
	delete[] stage1Dir;
	delete[] stage2Dir;

	coreFileSegments[0] = 0x0080;
	coreFileSectors[8] = kernel64Sector;
	coreFileSizes[8] = kernel64Size;
	for (int i = 1; i < moduleCount; i++) {
		coreFileSegments[i] = coreFileSegments[i - 1] + ceil((double)coreFileSizes[i - 1] / 16);
	}
	int bootBinSeekp = bootBinSector * ISO_SECTOR_SIZE + 32; // Skip first 32 bytes (contains int 0x22 routine)
	for (int i = 0; i < moduleCount; i++) {
		isoFile.seekp(bootBinSeekp + 2, ios::beg); // Skip first 2 bytes of each DAP
		uint16_t sectorCount = ceil((double)coreFileSizes[i] / ISO_SECTOR_SIZE); // DAP number of sectors to load
		isoFile.write((char *)&sectorCount, 2);
		uint16_t dapOffset = 0x0000; // Offset at which data is loaded is always 0x00
		isoFile.write((char *)&dapOffset, 2);
		if (strcmp(coreFiles[i], "ELFPARSE.BIN;1") == 0) {
			uint16_t elfParseSegment = 0x2000;
			isoFile.write((char *)&elfParseSegment, 2);
		} else if (strcmp(coreFiles[i], "LOADER32.;1") == 0 || strcmp(coreFiles[i], "KERNEL.;1") == 0) {
			uint16_t loader32ElfSegment = 0x8000;
			isoFile.write((char *)&loader32ElfSegment, 2);
		} else {
			isoFile.write((char *)&(coreFileSegments[i]), 2); // Segment at which data is loaded
		}
		isoFile.write((char *)&(coreFileSectors[i]), 4); // First sector of the file to be loaded
		bootBinSeekp += 16;
	}

	char *kernel = new char[kernel64Size];
	isoFile.seekg(kernel64Sector * ISO_SECTOR_SIZE, ios::beg);
	isoFile.read(kernel, kernel64Size);
	uint32_t programHeader = *((uint32_t *)(kernel + 32));
	uint16_t entrySize = *((uint16_t *)(kernel + 54));
	uint16_t entryCount = *((uint16_t *)(kernel + 56));
	uint64_t totalVirtualSize = 0;
	for (uint16_t i = 0; i < entryCount; i++) {
		uint32_t sectionType = *((uint32_t *)(kernel + programHeader + i * entrySize));
		if (sectionType == 1) {
			uint64_t virtualMemSize = *((uint64_t *)(kernel + programHeader + i * entrySize + 40));
			totalVirtualSize += ceil((double)virtualMemSize / 0x1000) * 0x1000; // Sections will be at 4KiB page boundary
		}
	}
	// Save the totalVirtualSize in BOOTLOAD.BIN after all the DAPs of the modules
	isoFile.seekp(bootBinSector * ISO_SECTOR_SIZE + 32 + moduleCount * 16, ios::beg);
	isoFile.write((char *)&totalVirtualSize, 8);
	delete[] kernel;

	cout << "ISO formatted" << endl;

	delete[] isoName;
	isoFile.close();
	return 0;
}
