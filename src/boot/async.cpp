#include <async.h>

void Async::Spinlock::lock() {
	while (flag.test_and_set(std::memory_order_acquire)) {
		while (flag.test(std::memory_order_relaxed)) {
			__builtin_ia32_pause();
		}
	}
}

void Async::Spinlock::unlock() {
	flag.clear();
}
