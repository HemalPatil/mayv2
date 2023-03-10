#include <cstring>
#include <drivers/storage/blockdevice.h>

static const char* const bufferNamespaceStr = "Storage::Buffer::";
static const char* const bufStr = "Buffer ";
static const char* const bufAllocErrorStr = "could not allocate buffer";
static const char* const wrongAlignStr = "wrong alignment";
static const char* const freeFailStr = "operator=(nullptr) failed to free buffer pages";

Drivers::Storage::Buffer::Buffer(std::nullptr_t) : data(nullptr), pageCount(0), physicalAddress((uint64_t)INVALID_ADDRESS), size(0) {}

Drivers::Storage::Buffer::Buffer(size_t size, size_t alignAt) {
	using namespace Kernel::Memory;

	this->size = size;
	this->pageCount = size / pageSize;
	if (size != this->pageCount * pageSize) {
		++this->pageCount;
	}
	PageRequestResult requestResult = Virtual::requestPages(
		this->pageCount,
		(
			RequestType::Kernel |	// TODO: maybe RequestType::Kernel is not required
			RequestType::PhysicalContiguous |
			RequestType::VirtualContiguous |
			RequestType::AllocatePhysical |
			RequestType::CacheDisable
		)
	);
	if (requestResult.address == INVALID_ADDRESS || requestResult.allocatedCount != this->pageCount) {
		terminalPrintString(bufferNamespaceStr, strlen(bufferNamespaceStr));
		terminalPrintString(bufStr, strlen(bufStr));
		terminalPrintString(bufAllocErrorStr, strlen(bufAllocErrorStr));
		Kernel::panic();
	}
	this->data = requestResult.address;

	// Make sure buffer's physical address is aligned at correct boundary
	Kernel::Memory::Virtual::CrawlResult crawlResult(this->data);
	uint64_t physicalAddress = (uint64_t)crawlResult.physicalTables[0] + crawlResult.indexes[0];
	if (
		crawlResult.physicalTables[0] == INVALID_ADDRESS ||
		physicalAddress % alignAt != 0
	) {
		terminalPrintString(bufferNamespaceStr, strlen(bufferNamespaceStr));
		terminalPrintString(bufStr, strlen(bufStr));
		terminalPrintString(wrongAlignStr, strlen(wrongAlignStr));
		Kernel::panic();
	}
	this->physicalAddress = physicalAddress;
}

Drivers::Storage::Buffer::Buffer(Buffer &&other)
	:	data(other.data),
		pageCount(other.pageCount),
		physicalAddress(other.physicalAddress),
		size(other.size) {
	other.data = nullptr;
	other.pageCount = other.size = 0;
	other.physicalAddress = (uint64_t)INVALID_ADDRESS;
}

Drivers::Storage::Buffer::~Buffer() {
	*this = nullptr;
}

Drivers::Storage::Buffer& Drivers::Storage::Buffer::operator=(std::nullptr_t) {
	using namespace Kernel::Memory;

	if (
		this->data &&
		this->pageCount != 0 &&
		!Virtual::freePages(this->data, this->pageCount, RequestType::Kernel | RequestType::AllocatePhysical)
	) {
		terminalPrintString(bufferNamespaceStr, strlen(bufferNamespaceStr));
		terminalPrintString(freeFailStr, strlen(freeFailStr));
		Kernel::panic();
	}
	this->data = nullptr;
	this->pageCount = this->size = 0;
	this->physicalAddress = (uint64_t)INVALID_ADDRESS;
	return *this;
}

Drivers::Storage::Buffer& Drivers::Storage::Buffer::operator=(Buffer &&other) {
	*this = nullptr;
	this->data = other.data;
	this->pageCount = other.pageCount;
	this->physicalAddress = other.physicalAddress;
	this->size = other.size;
	other.data = nullptr;
	other.pageCount = other.size = 0;
	other.physicalAddress = (uint64_t)INVALID_ADDRESS;
	return *this;
}

void* Drivers::Storage::Buffer::getData() const {
	return this->data;
}

size_t Drivers::Storage::Buffer::getSize() const {
	return this->size;
}

Drivers::Storage::Buffer::operator bool() const {
	return this->data != nullptr;
}

uint64_t Drivers::Storage::Buffer::getPhysicalAddress() const {
	return this->physicalAddress;
}

size_t Drivers::Storage::BlockDevice::getBlockSize() const {
	return this->blockSize;
}
