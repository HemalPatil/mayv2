#pragma once

#include <drivers/storage/blockdevice.h>

namespace FS {
	class BaseFS {
		protected:
			Storage::BlockDevice &device;

		public:
			BaseFS(Storage::BlockDevice &blockDevice) : device(blockDevice) {}
			virtual bool openFile() = 0;
	};
}
