#pragma once

namespace std {
	// Helpers for exception objects in <stdexcept>
	void
  __throw_out_of_range_fmt(const char*, ...) __attribute__((__noreturn__))
    __attribute__((__format__(__gnu_printf__, 1, 2)));

	// Helpers for exception objects in <functional>
	void
  __throw_bad_function_call() __attribute__((__noreturn__));

	// Helper for exception objects in <new>
	void
  __throw_bad_alloc(void) __attribute__((__noreturn__));

	void
  __throw_bad_array_new_length(void) __attribute__((__noreturn__));
}

