#pragma once

#include <kernel.h>

namespace Kernel {
	template<typename T>
	class Promise {
		private:
			volatile bool valueSet = false;
			T value;

		public:
			T get() {
				while (!valueSet) {
					haltSystem(false);
				}
				return value;
			}

			void setValue(T value) {
				this->value = value;
				this->valueSet = true;
			}
	};
}
