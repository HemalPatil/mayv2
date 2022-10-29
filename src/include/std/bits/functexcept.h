#pragma once

namespace std {
	// Helpers for exception objects in <stdexcept>
	[[noreturn]] void __throw_invalid_argument(const char*);
	[[noreturn]] void __throw_logic_error(const char*);
	[[noreturn]] void __throw_length_error(const char*);
	[[noreturn]] void __throw_out_of_range(const char*);
	[[noreturn]] void __throw_out_of_range_fmt(const char*, ...) __attribute__((__format__(__gnu_printf__, 1, 2)));

	// Helpers for exception objects in <functional>
	[[noreturn]] void __throw_bad_function_call();

	// Helper for exception objects in <new>
	[[noreturn]] void __throw_bad_alloc(void);
	[[noreturn]] void __throw_bad_array_new_length(void);
}

