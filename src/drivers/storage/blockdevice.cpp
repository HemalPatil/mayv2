#include <drivers/storage/blockdevice.h>
#include <string.h>

static const char* const bufAllocErrorStr = "Storage::Buffer::Buffer could not allocate buffer or wrong alignment";
static const char* const freeFailStr = "Storage::Buffer::operator= failed to free buffer pages";

Storage::Buffer::Buffer(std::nullptr_t) : data(nullptr), pageCount(SIZE_MAX), physicalAddress(UINT64_MAX) {}

Storage::Buffer::Buffer(size_t size, size_t alignAt) {
	using namespace Kernel::Memory;

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
		terminalPrintString(bufAllocErrorStr, strlen(bufAllocErrorStr));
		Kernel::panic();
	}
	this->data = requestResult.address;

	// Make sure buffer's physical address is aligned at correct boundary
	Kernel::Memory::Virtual::CrawlResult crawlResult(this->data);
	if (crawlResult.physicalTables[0] == INVALID_ADDRESS || crawlResult.indexes[0] % alignAt != 0) {
		terminalPrintString(bufAllocErrorStr, strlen(bufAllocErrorStr));
		Kernel::panic();
	}
	this->physicalAddress = (uint64_t)crawlResult.physicalTables[0] + crawlResult.indexes[0];
}

Storage::Buffer::Buffer(Buffer &&other) : data(other.data), pageCount(other.pageCount), physicalAddress(other.physicalAddress) {
	other.data = nullptr;
	other.pageCount = SIZE_MAX;
	other.physicalAddress = UINT64_MAX;
}

Storage::Buffer::~Buffer() {
	*this = nullptr;
}

Storage::Buffer& Storage::Buffer::operator=(std::nullptr_t) {
	using namespace Kernel::Memory;

	if (
		this->data &&
		this->pageCount != SIZE_MAX &&
		!Virtual::freePages(this->data, this->pageCount, RequestType::Kernel | RequestType::AllocatePhysical)
	) {
		terminalPrintString(freeFailStr, strlen(freeFailStr));
		Kernel::panic();
	}
	this->data = nullptr;
	this->pageCount = SIZE_MAX;
	this->physicalAddress = UINT64_MAX;
	return *this;
}

Storage::Buffer& Storage::Buffer::operator=(Buffer &&other) {
	*this = nullptr;
	this->data = other.data;
	this->pageCount = other.pageCount;
	this->physicalAddress = other.physicalAddress;
	other.data = nullptr;
	other.pageCount = SIZE_MAX;
	other.physicalAddress = UINT64_MAX;
	return *this;
}

void* Storage::Buffer::getData() const {
	return this->data;
}

Storage::Buffer::operator bool() const {
	return this->data != nullptr;
}

uint64_t Storage::Buffer::getPhysicalAddress() const {
	return this->physicalAddress;
}

size_t Storage::BlockDevice::getBlockSize() const {
	return this->blockSize;
}
