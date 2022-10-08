#pragma once

#include <stddef.h>

// namespace detail {
// template <class T> constexpr T& FUN(T& t) noexcept { return t; }
// template <class T> void FUN(T&&) = delete;
// }

namespace std {
	// reference_wrapper, forward declaration
	template<typename _Tp>
    class reference_wrapper;

	// void_t
	template<typename...> using void_t = void;

	// enable_if, enable_if_t
	template<bool, typename Type = void> struct enable_if { };
	template<typename _Tp> struct enable_if<true, _Tp> { typedef _Tp type; };
	template<bool Condition, typename Type = void>
	using enable_if_t = typename enable_if<Condition, Type>::type;

	// integral_constant
	template<typename Type, Type Value>
	struct integral_constant {
		static constexpr Type value = Value;
		typedef Type value_type;
		typedef integral_constant<Type, Value> type;
		constexpr operator value_type() const noexcept { return value; }
		constexpr value_type operator()() const noexcept { return value; }
	};

	// true_type, false_type
	using true_type =  integral_constant<bool, true>;
	using false_type = integral_constant<bool, false>;

	// bool_constant
	template<bool B>
	using bool_constant = integral_constant<bool, B>;

	// conditional, conditional_t
	template<bool, typename, typename> struct conditional;
	template<bool Condition, typename IfTrue, typename IfFalse> struct conditional { typedef IfTrue type; };
	template<typename IfTrue, typename IfFalse> struct conditional<false, IfTrue, IfFalse> { typedef IfFalse type; };
	template<bool Condition, typename IfTrue, typename IfFalse>
	using conditional_t = typename conditional<Condition, IfTrue, IfFalse>::type;

	// undocumented or, and, not, __type_identity, __type_identity_t
	template<typename...> struct __or_;
	template<> struct __or_<> : public false_type { };
	template<typename B1> struct __or_<B1> : public B1 { };
	template<typename B1, typename B2> struct __or_<B1, B2> : public conditional<B1::value, B1, B2>::type { };
	template<typename B1, typename B2, typename B3, typename... Bn> struct __or_<B1, B2, B3, Bn...> : public conditional<B1::value, B1, __or_<B2, B3, Bn...>>::type { };
	template<typename...> struct __and_;
	template<> struct __and_<> : public true_type { };
	template<typename B1> struct __and_<B1> : public B1 { };
	template<typename B1, typename B2> struct __and_<B1, B2> : public conditional<B1::value, B2, B1>::type { };
	template<typename B1, typename B2, typename B3, typename... Bn> struct __and_<B1, B2, B3, Bn...> : public conditional<B1::value, __and_<B2, B3, Bn...>, B1>::type { };
	template<typename Value> struct __not_ : public bool_constant<!bool(Value::value)> { };
	template <typename Type> struct __type_identity { using type = Type; };
	template<typename Type> using __type_identity_t = typename __type_identity<Type>::type;

	// _Require for SFINAE constraints
	template<typename... Condition>
	using _Require = enable_if_t<__and_<Condition...>::value>;

	// remove_const, remove_const_t
	template<typename Type> struct remove_const { typedef Type type; };
	template<typename Type> struct remove_const<Type const> { typedef Type type; };
	template<typename Type>
	using remove_const_t = typename remove_const<Type>::type;

	// remove_volatile, remove_volatile_t
	template<typename Type> struct remove_volatile { typedef Type type; };
	template<typename Type> struct remove_volatile<Type volatile> { typedef Type type; };
	template<typename Type>
	using remove_volatile_t = typename remove_volatile<Type>::type;

	// remove_cv, remove_cv_t
	template<typename Type> struct remove_cv { using type = Type; };
	template<typename Type> struct remove_cv<const Type> { using type = Type; };
	template<typename Type> struct remove_cv<volatile Type> { using type = Type; };
	template<typename Type> struct remove_cv<const volatile Type> { using type = Type; };
	template<typename _Tp>
	using remove_cv_t = typename remove_cv<_Tp>::type;

	// remove_cvref, remove_cvref_t
	template<typename _Tp> struct remove_cvref : remove_cv<_Tp> { };
	template<typename _Tp> struct remove_cvref<_Tp&> : remove_cv<_Tp> { };
	template<typename _Tp> struct remove_cvref<_Tp&&> : remove_cv<_Tp> { };
	template<typename _Tp> using remove_cvref_t = typename remove_cvref<_Tp>::type;

	// remove_extent, remove_extent_t
	template<typename T> struct remove_extent { typedef T type; };
	template<typename T> struct remove_extent<T[]> { typedef T type; };
	template<typename T, size_t N> struct remove_extent<T[N]> { typedef T type; };
	template<typename T>
	using remove_extent_t = typename remove_extent<T>::type;

	// remove_reference, remove_reference_t
	template <typename Type> struct remove_reference { typedef Type type; };
	template <typename Type> struct remove_reference<Type &> { typedef Type type; };
	template <typename Type> struct remove_reference<Type &&> { typedef Type type; };
	template<typename Type>
	using remove_reference_t = typename remove_reference<Type>::type;

	// is_base_of
	template<typename _Base, typename _Derived> struct is_base_of : public integral_constant<bool, __is_base_of(_Base, _Derived)> { };

	// is_same, is_same_v
	template<typename _Tp, typename _Up> struct is_same : public false_type { };
	template<typename _Tp> struct is_same<_Tp, _Tp> : public true_type { };
	template <typename _Tp, typename _Up> inline constexpr bool is_same_v = is_same<_Tp, _Up>::value;

	// is_array
	template<typename> struct is_array : public false_type { };
	template<typename Type, size_t Size> struct is_array<Type[Size]> : public true_type { };
	template<typename Type> struct is_array<Type[]> : public true_type { };

	// __is_pointer_helper
	template<typename> struct __is_pointer_helper : public false_type { };
	template<typename Type> struct __is_pointer_helper<Type*> : public true_type { };

	// is_pointer
	template<typename Type> struct is_pointer : public __is_pointer_helper<remove_cv_t<Type>>::type { };

	// is_lvalue_reference, is_lvalue_reference_v
	template<typename> struct is_lvalue_reference : public false_type { };
	template<typename Type> struct is_lvalue_reference<Type&> : public true_type { };
	template <typename Type>
	inline constexpr bool is_lvalue_reference_v = is_lvalue_reference<Type>::value;

	// is_rvalue_reference, is_rvalue_reference_v
	template<typename> struct is_rvalue_reference : public false_type { };
	template<typename _Tp> struct is_rvalue_reference<_Tp&&> : public true_type { };
	template <typename _Tp>
	inline constexpr bool is_rvalue_reference_v = is_rvalue_reference<_Tp>::value;

	// is_reference
	template<typename _Tp>
	struct is_reference : public __or_<is_lvalue_reference<_Tp>, is_rvalue_reference<_Tp>>::type { };

	// __is_void_helper
	template<typename> struct __is_void_helper : public false_type { };
	template<> struct __is_void_helper<void> : public true_type { };

	// is_void
	template<typename Type> struct is_void : public __is_void_helper<remove_cv_t<Type>>::type { };

	// is_const
	template<typename> struct is_const : public false_type { };
	template<typename Type> struct is_const<Type const> : public true_type { };

	// is_function
	template<typename Type> struct is_function : public bool_constant<!is_const<const Type>::value> { };

	// __is_convertible_helper
	template<typename From, typename To, bool = __or_<is_void<From>, is_function<To>, is_array<To>>::value>
	struct __is_convertible_helper {
		typedef typename is_void<To>::type type;
	};

	// is_convertible
	template<typename From, typename To>
	struct is_convertible : public __is_convertible_helper<From, To>::type { };
	template <typename From, typename To>
	inline constexpr bool is_convertible_v = is_convertible<From, To>::value;

	// is_array_convertible for unique_ptr<T[]>, shared_ptr<T[]>, and span<T, N>
	template<typename ToElementType, typename FromElementType>
	using is_array_convertible = is_convertible<FromElementType(*)[], ToElementType(*)[]>;

	// __is_member_object_pointer_helper
	template<typename> struct __is_member_object_pointer_helper : public false_type { };
	template<typename _Tp, typename _Cp> struct __is_member_object_pointer_helper<_Tp _Cp::*> : public __not_<is_function<_Tp>>::type { };

	// is_member_object_pointer
	template<typename _Tp> struct is_member_object_pointer : public __is_member_object_pointer_helper<remove_cv_t<_Tp>>::type { };

	// __is_member_function_pointer_helper
	template<typename> struct __is_member_function_pointer_helper : public false_type { };
	template<typename _Tp, typename _Cp> struct __is_member_function_pointer_helper<_Tp _Cp::*> : public is_function<_Tp>::type { };

	// is_member_function_pointer
	template<typename _Tp> struct is_member_function_pointer : public __is_member_function_pointer_helper<remove_cv_t<_Tp>>::type { };

	// extent, extent_v
	template<typename, unsigned = 0> struct extent;
	template<typename, unsigned _Uint> struct extent : public integral_constant<size_t, 0> { };
	template<typename _Tp, unsigned _Uint, size_t _Size>
	struct extent<_Tp[_Size], _Uint> : public integral_constant<size_t, _Uint == 0 ? _Size : extent<_Tp, _Uint - 1>::value> { };
	template<typename _Tp, unsigned _Uint>
	struct extent<_Tp[], _Uint> : public integral_constant<size_t, _Uint == 0 ? 0 : extent<_Tp, _Uint - 1>::value> { };
	template <typename _Tp, unsigned _Idx = 0> inline constexpr size_t extent_v = extent<_Tp, _Idx>::value;

	// __is_complete_or_unbounded
	// Helper functions that return false_type for incomplete classes,
	// incomplete unions and arrays of known bound from those.
	template <typename _Tp, size_t = sizeof(_Tp)>
	constexpr true_type __is_complete_or_unbounded(__type_identity<_Tp>) { return {}; }

	// __is_array_unknown_bounds
	template<typename _Tp>
	struct __is_array_unknown_bounds : public __and_<is_array<_Tp>, __not_<extent<_Tp>>> { };

	template <typename _TypeIdentity, typename _NestedType = typename _TypeIdentity::type>
	constexpr typename __or_<
		is_reference<_NestedType>,
		is_function<_NestedType>,
		is_void<_NestedType>,
		__is_array_unknown_bounds<_NestedType>
	>::type __is_complete_or_unbounded(_TypeIdentity) { return {}; }

	// __is_referenceable
	template<typename Type, typename = void>
	struct __is_referenceable : public false_type { };

	template<typename Type>
	struct __is_referenceable<Type, void_t<Type&>> : public true_type { };

	// __is_constructible_impl
	template<typename Type, typename... Args>
	struct __is_constructible_impl : public bool_constant<__is_constructible(Type, Args...)> { };

	// __is_move_constructible_impl
	template<typename Type, bool = __is_referenceable<Type>::value> struct __is_move_constructible_impl;
	template<typename Type> struct __is_move_constructible_impl<Type, false> : public false_type { };
	template<typename Type> struct __is_move_constructible_impl<Type, true> : public __is_constructible_impl<Type, Type&&> { };

	// is_move_constructible, is_move_constructible_v
	template<typename Type>
	struct is_move_constructible : public __is_move_constructible_impl<Type> {
		static_assert(
			std::__is_complete_or_unbounded(__type_identity<Type>{}),
			"template argument must be a complete struct or an unbounded array"
		);
	};
	template <typename Type>
	inline constexpr bool is_move_constructible_v = is_move_constructible<Type>::value;

	// __is_copy_constructible_impl
	template<typename _Tp, bool = __is_referenceable<_Tp>::value> struct __is_copy_constructible_impl;
	template<typename _Tp> struct __is_copy_constructible_impl<_Tp, false> : public false_type { };
	template<typename _Tp> struct __is_copy_constructible_impl<_Tp, true> : public __is_constructible_impl<_Tp, const _Tp&> { };

	// is_copy_constructible, is_copy_constructible_v
	template<typename _Tp>
	struct is_copy_constructible : public __is_copy_constructible_impl<_Tp> {
		static_assert(
			std::__is_complete_or_unbounded(__type_identity<_Tp>{}),
			"template argument must be a complete struct or an unbounded array"
		);
	};
	template <typename _Tp>
	inline constexpr bool is_copy_constructible_v = is_copy_constructible<_Tp>::value;

	// is_assignable
	template<typename _Tp, typename _Up>
	struct is_assignable : public bool_constant<__is_assignable(_Tp, _Up)> {
		static_assert(
			std::__is_complete_or_unbounded(__type_identity<_Tp>{}),
			"template argument must be a complete struct or an unbounded array"
		);
	};

	// __is_move_assignable_impl
	template<typename _Tp, bool = __is_referenceable<_Tp>::value> struct __is_move_assignable_impl;
	template<typename _Tp> struct __is_move_assignable_impl<_Tp, false> : public false_type { };
	template<typename _Tp> struct __is_move_assignable_impl<_Tp, true> : public bool_constant<__is_assignable(_Tp&, _Tp&&)> { };

	// is_move_assignable, is_move_assignable_v
	template<typename _Tp>
	struct is_move_assignable : public __is_move_assignable_impl<_Tp>::type {
		static_assert(
			std::__is_complete_or_unbounded(__type_identity<_Tp>{}),
			"template argument must be a complete struct or an unbounded array"
		);
	};
	template <typename _Tp>
	inline constexpr bool is_move_assignable_v = is_move_assignable<_Tp>::value;

	// is_default_constructible, is_default_constructible_v
	template<typename _Tp>
	struct is_default_constructible : public __is_constructible_impl<_Tp>::type {
		static_assert(
			std::__is_complete_or_unbounded(__type_identity<_Tp>{}),
			"template argument must be a complete struct or an unbounded array"
		);
	};
	template <typename _Tp>
	inline constexpr bool is_default_constructible_v = is_default_constructible<_Tp>::value;

	// __add_lvalue_reference_helper
	template<typename _Tp, bool = __is_referenceable<_Tp>::value> struct __add_lvalue_reference_helper { typedef _Tp   type; };
	template<typename _Tp> struct __add_lvalue_reference_helper<_Tp, true> { typedef _Tp&   type; };

	// add_lvalue_reference, add_lvalue_reference_t
	template<typename _Tp> struct add_lvalue_reference : public __add_lvalue_reference_helper<_Tp> { };
	template<typename _Tp> using add_lvalue_reference_t = typename add_lvalue_reference<_Tp>::type;

	// __add_rvalue_reference_helper
	template<typename _Tp, bool = __is_referenceable<_Tp>::value> struct __add_rvalue_reference_helper { typedef _Tp   type; };
	template<typename _Tp> struct __add_rvalue_reference_helper<_Tp, true> { typedef _Tp&&   type; };

	// add_rvalue_reference, add_rvalue_reference_t
	template<typename _Tp> struct add_rvalue_reference : public __add_rvalue_reference_helper<_Tp> { };
	template<typename _Tp> using add_rvalue_reference_t = typename add_rvalue_reference<_Tp>::type;

	// __add_pointer_helper
	template<typename _Tp, bool = __or_<__is_referenceable<_Tp>, is_void<_Tp>>::value> struct __add_pointer_helper { typedef _Tp type; };
	template<typename _Tp> struct __add_pointer_helper<_Tp, true> { typedef typename remove_reference<_Tp>::type* type; };

	// add_pointer, add_pointer_t
	template<typename _Tp> struct add_pointer : public __add_pointer_helper<_Tp> { };
	template<typename _Tp> using add_pointer_t = typename add_pointer<_Tp>::type;

	// Decay trait for arrays and functions, used for perfect forwarding
  // in make_pair, make_tuple, etc.
  template<typename _Up,
	   bool _IsArray = is_array<_Up>::value,
	   bool _IsFunction = is_function<_Up>::value>
    struct __decay_selector;

  // NB: DR 705.
  template<typename _Up>
    struct __decay_selector<_Up, false, false>
    { typedef remove_cv_t<_Up> __type; };

  template<typename _Up>
    struct __decay_selector<_Up, true, false>
    { typedef typename remove_extent<_Up>::type* __type; };

  template<typename _Up>
    struct __decay_selector<_Up, false, true>
    { typedef typename add_pointer<_Up>::type __type; };

	// decay, decay_t
	template<typename _Tp>
	struct decay {
		private:
			typedef typename remove_reference<_Tp>::type __remove_type;
		public:
			typedef typename __decay_selector<__remove_type>::__type type;
	};
	template<typename _Tp> using decay_t = typename decay<_Tp>::type;

	// __success_type, __failure_type
	template<typename _Tp> struct __success_type { typedef _Tp type; };
	struct __failure_type { };

	// declval
	template<typename T> constexpr bool always_false = false;
template<typename T>
typename std::add_rvalue_reference<T>::type declval() noexcept {
    static_assert(always_false<T>, "declval not allowed in an evaluated context");
}

	struct __invoke_memfun_ref { };
  struct __invoke_memfun_deref { };
  struct __invoke_memobj_ref { };
  struct __invoke_memobj_deref { };
  struct __invoke_other { };

	// Associate a tag type with a specialization of __success_type.
  template<typename _Tp, typename _Tag>
    struct __result_of_success : __success_type<_Tp>
    { using __invoke_type = _Tag; };

  // [func.require] paragraph 1 bullet 1:
  struct __result_of_memfun_ref_impl
  {
    template<typename _Fp, typename _Tp1, typename... _Args>
      static __result_of_success<decltype(
      (std::declval<_Tp1>().*std::declval<_Fp>())(std::declval<_Args>()...)
      ), __invoke_memfun_ref> _S_test(int);

    template<typename...>
      static __failure_type _S_test(...);
  };

  template<typename _MemPtr, typename _Arg, typename... _Args>
    struct __result_of_memfun_ref
    : private __result_of_memfun_ref_impl
    {
      typedef decltype(_S_test<_MemPtr, _Arg, _Args...>(0)) type;
    };

  // [func.require] paragraph 1 bullet 2:
  struct __result_of_memfun_deref_impl
  {
    template<typename _Fp, typename _Tp1, typename... _Args>
      static __result_of_success<decltype(
      ((*std::declval<_Tp1>()).*std::declval<_Fp>())(std::declval<_Args>()...)
      ), __invoke_memfun_deref> _S_test(int);

    template<typename...>
      static __failure_type _S_test(...);
  };

  template<typename _MemPtr, typename _Arg, typename... _Args>
    struct __result_of_memfun_deref
    : private __result_of_memfun_deref_impl
    {
      typedef decltype(_S_test<_MemPtr, _Arg, _Args...>(0)) type;
    };

	// [func.require] paragraph 1 bullet 3:
  struct __result_of_memobj_ref_impl
  {
    template<typename _Fp, typename _Tp1>
      static __result_of_success<decltype(
      std::declval<_Tp1>().*std::declval<_Fp>()
      ), __invoke_memobj_ref> _S_test(int);

    template<typename, typename>
      static __failure_type _S_test(...);
  };

  template<typename _MemPtr, typename _Arg>
    struct __result_of_memobj_ref
    : private __result_of_memobj_ref_impl
    {
      typedef decltype(_S_test<_MemPtr, _Arg>(0)) type;
    };

  // [func.require] paragraph 1 bullet 4:
  struct __result_of_memobj_deref_impl
  {
    template<typename _Fp, typename _Tp1>
      static __result_of_success<decltype(
      (*std::declval<_Tp1>()).*std::declval<_Fp>()
      ), __invoke_memobj_deref> _S_test(int);

    template<typename, typename>
      static __failure_type _S_test(...);
  };

  template<typename _MemPtr, typename _Arg>
    struct __result_of_memobj_deref
    : private __result_of_memobj_deref_impl
    {
      typedef decltype(_S_test<_MemPtr, _Arg>(0)) type;
    };

  template<typename _MemPtr, typename _Arg>
    struct __result_of_memobj;

  template<typename _Res, typename _Class, typename _Arg>
    struct __result_of_memobj<_Res _Class::*, _Arg>
    {
      typedef remove_cvref_t<_Arg> _Argval;
      typedef _Res _Class::* _MemPtr;
      typedef typename conditional<__or_<is_same<_Argval, _Class>,
        is_base_of<_Class, _Argval>>::value,
        __result_of_memobj_ref<_MemPtr, _Arg>,
        __result_of_memobj_deref<_MemPtr, _Arg>
      >::type::type type;
    };

	template<typename _MemPtr, typename _Arg, typename... _Args>
    struct __result_of_memfun;

  template<typename _Res, typename _Class, typename _Arg, typename... _Args>
    struct __result_of_memfun<_Res _Class::*, _Arg, _Args...>
    {
      typedef typename remove_reference<_Arg>::type _Argval;
      typedef _Res _Class::* _MemPtr;
      typedef typename conditional<is_base_of<_Class, _Argval>::value,
        __result_of_memfun_ref<_MemPtr, _Arg, _Args...>,
        __result_of_memfun_deref<_MemPtr, _Arg, _Args...>
      >::type::type type;
    };

// 	template <class T>
// class reference_wrapper {
// public:
//   // types
//   typedef T type;
 
//   // construct/copy/destroy
//   template <class U, class = decltype(
//     detail::FUN<T>(std::declval<U>()),
//     std::enable_if_t<!std::is_same_v<reference_wrapper, std::remove_cvref_t<U>>>()
//   )>
//   constexpr reference_wrapper(U&& u) noexcept(noexcept(detail::FUN<T>(std::forward<U>(u))))
//     : _ptr(std::addressof(detail::FUN<T>(std::forward<U>(u)))) {}
//   reference_wrapper(const reference_wrapper&) noexcept = default;
 
//   // assignment
//   reference_wrapper& operator=(const reference_wrapper& x) noexcept = default;
 
//   // access
//   constexpr operator T& () const noexcept { return *_ptr; }
//   constexpr T& get() const noexcept { return *_ptr; }
 
//   template< class... ArgTypes >
//   constexpr std::invoke_result_t<T&, ArgTypes...>
//     operator() ( ArgTypes&&... args ) const {
//     return std::invoke(get(), std::forward<ArgTypes>(args)...);
//   }
 
// private:
//   T* _ptr;
// };

// 	// __inv_unwrap, used by result_of, invoke etc. to unwrap a reference_wrapper.
//   template<typename _Tp, typename _Up = remove_cvref_t<_Tp>>
//     struct __inv_unwrap
//     {
//       using type = _Tp;
//     };

//   template<typename _Tp, typename _Up>
//     struct __inv_unwrap<_Tp, reference_wrapper<_Up>>
//     {
//       using type = _Up&;
//     };

// 	// __result_of_impl
// 	template<bool, bool, typename _Functor, typename... _ArgTypes> struct __result_of_impl { typedef __failure_type type; };
// 	template<typename _MemPtr, typename _Arg>
// 	struct __result_of_impl<true, false, _MemPtr, _Arg> : public __result_of_memobj<decay_t<_MemPtr>, typename __inv_unwrap<_Arg>::type> { };
// 	template<typename _MemPtr, typename _Arg, typename... _Args>
// 	struct __result_of_impl<false, true, _MemPtr, _Arg, _Args...>
// 	: public __result_of_memfun<decay_t<_MemPtr>, typename __inv_unwrap<_Arg>::type, _Args...> { };
// 	struct __result_of_other_impl {
// 		template<typename _Fn, typename... _Args>
// 		static __result_of_success<decltype(std::declval<_Fn>()(std::declval<_Args>()...)), __invoke_other> _S_test(int);
// 		template<typename...>
// 		static __failure_type _S_test(...);
// 	};
// 	template<typename _Functor, typename... _ArgTypes>
// 	struct __result_of_impl<false, false, _Functor, _ArgTypes...> : private __result_of_other_impl {
// 		typedef decltype(_S_test<_Functor, _ArgTypes...>(0)) type;
// 	};

// 	// __invoke_result
// 	template<typename _Functor, typename... _ArgTypes>
// 	struct __invoke_result : public __result_of_impl<
// 		is_member_object_pointer<typename remove_reference<_Functor>::type>::value,
// 		is_member_function_pointer<typename remove_reference<_Functor>::type>::value,
// 		_Functor, _ArgTypes...
// 	>::type { };

// 	// invoke_result, invoke_result_t
// 	template<typename _Functor, typename... _ArgTypes>
// 	struct invoke_result : public __invoke_result<_Functor, _ArgTypes...> {
// 		static_assert(
// 			std::__is_complete_or_unbounded(__type_identity<_Functor>{}),
// 			"_Functor must be a complete struct or an unbounded array"
// 		);
// 		static_assert(
// 			(std::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
// 			"each argument type must be a complete struct or an unbounded array"
// 		);
// 	};
// 	template<typename _Fn, typename... _Args> using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;

// 	// __is_invocable_impl
// 	template<typename _Result, typename _Ret, bool = is_void<_Ret>::value, typename = void> struct __is_invocable_impl : false_type { };
// 	template<typename _Result, typename _Ret> struct __is_invocable_impl<_Result, _Ret, true, void_t<typename _Result::type>> : true_type { };

// 	// __is_invocable
// 	template<typename _Fn, typename... _ArgTypes> struct __is_invocable : __is_invocable_impl<__invoke_result<_Fn, _ArgTypes...>, void>::type { };

// 	// is_invocable
// 	template<typename _Fn, typename... _ArgTypes>
// 	struct is_invocable : __is_invocable_impl<__invoke_result<_Fn, _ArgTypes...>, void>::type {
// 		static_assert(
// 			std::__is_complete_or_unbounded(__type_identity<_Fn>{}),
// 			"_Fn must be a complete struct or an unbounded array"
// 		);
// 		static_assert(
// 			(std::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
// 			"each argument type must be a complete struct or an unbounded array"
// 		);
// 	};
}
