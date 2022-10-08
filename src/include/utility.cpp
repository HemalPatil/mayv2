#pragma once

#include <type_traits.cpp>

namespace std {
	// Forward an lvalue
	template<typename Type>
	[[nodiscard]] constexpr Type&& forward(typename std::remove_reference<Type>::type& type) noexcept {
		return static_cast<Type&&>(type);
	}

	// Forward an rvalue
	template<typename Type>
	[[nodiscard]] constexpr Type&& forward(typename std::remove_reference<Type>::type&& type) noexcept {
		static_assert(
			!std::is_lvalue_reference<Type>::value,
			"std::forward must not be used to convert an rvalue to an lvalue"
		);
		return static_cast<Type&&>(type);
	}

	// Move
	template<typename Type>
	[[nodiscard]] constexpr typename std::remove_reference<Type>::type&& move(Type&& type) noexcept {
		return static_cast<typename std::remove_reference<Type>::type&&>(type);
	}

	// Swap
	template<typename _Tp>
	constexpr void swap(_Tp& __a, _Tp& __b) noexcept {
		_Tp __tmp = move(__a);
		__a = move(__b);
		__b = move(__tmp);
	}

	template<typename _Tp, size_t _Nm>
	constexpr void swap(_Tp (&__a)[_Nm], _Tp (&__b)[_Nm]) noexcept {
		for (size_t __n = 0; __n < _Nm; ++__n) {
			swap(__a[__n], __b[__n]);
		}
	}
}
