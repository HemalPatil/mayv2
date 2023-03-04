#pragma once

#include <cstdint>
#include <string>

namespace Random {
	class GUIDv4 {
		private:
			uint32_t timeLow;
			uint16_t timeMid;
			uint16_t timeHighAndVersion;
			uint16_t clockSequenceAndVariant;
			uint64_t node;

		public:
			GUIDv4();
			bool operator==(const GUIDv4 &other) const;
			explicit operator std::string() const;
			void print(bool showBraces = false) const;
	};

	extern "C" uint64_t getRandom64();
	extern "C" bool isRandomAvailable();
}
