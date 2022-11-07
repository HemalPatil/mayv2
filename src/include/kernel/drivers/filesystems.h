#pragma once

#include <drivers/storage/blockdevice.h>
#include <vector>

namespace FS {
	class BaseFS {
		protected:
			Storage::BlockDevice &device;

		public:
			BaseFS *next = nullptr;

			BaseFS(Storage::BlockDevice &blockDevice) : device(blockDevice) {}
			virtual bool openFile() = 0;

			static void createFs(Storage::BlockDevice &device);
	};

	extern std::vector<std::shared_ptr<BaseFS>> filesystems;
}
