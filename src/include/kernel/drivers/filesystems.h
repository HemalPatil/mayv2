#pragma once

#include <drivers/storage/blockdevice.h>
#include <vector>

namespace FS {
	class BaseFS {
		protected:
			std::shared_ptr<Storage::BlockDevice> device;

		public:
			BaseFS(std::shared_ptr<Storage::BlockDevice> blockDevice) : device(blockDevice) {}
			virtual bool openFile() = 0;
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;
}
