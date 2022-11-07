#pragma once

#include <kernel.h>

namespace Kernel {
	template<typename T>
	class Promise {
		private:
			volatile bool valueSet = false;
			T value;

		public:
			T awaitGet() {
				while (!valueSet) {
					haltSystem(false);
				}
				return value;
			}

			void setValue(T value) {
				this->value = value;
				this->valueSet = true;
			}

			static Promise<T> resolved(T value) {
				Promise<T> promise;
				promise.setValue(value);
				return promise;
			}
	};
}
