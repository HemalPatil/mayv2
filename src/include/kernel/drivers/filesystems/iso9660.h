#pragma once

#include <drivers/filesystems.h>

namespace FS {
	class ISO9660 : public BaseFS {
		public:
			static bool isIso9660(Storage::BlockDevice *device);
	};
}
