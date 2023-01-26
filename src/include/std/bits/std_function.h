#pragma once

#include <bits/functexcept.h>
#include <bits/invoke.h>
#include <bits/refwrap.h>
#include <cstddef>
#include <new>

namespace std {

	template <typename T>
	class function;

	template<typename _Tp>
    struct __is_location_invariant
    : is_trivially_copyable<_Tp>::type
    { };

	class _Undefined_class;

	union _Nocopy_types
  {
    void*       _M_object;
    const void* _M_const_object;
    void (*_M_function_pointer)();
    void (_Undefined_class::*_M_member_pointer)();
  };

  union [[gnu::may_alias]] _Any_data
  {
    void*       _M_access()       { return &_M_pod_data[0]; }
    const void* _M_access() const { return &_M_pod_data[0]; }

    template<typename _Tp>
      _Tp&
      _M_access()
      { return *static_cast<_Tp*>(_M_access()); }

    template<typename _Tp>
      const _Tp&
      _M_access() const
      { return *static_cast<const _Tp*>(_M_access()); }

    _Nocopy_types _M_unused;
    char _M_pod_data[sizeof(_Nocopy_types)];
  };

	enum _Manager_operation
  {
    // __get_type_info,
    __get_functor_ptr,
    __clone_functor,
    __destroy_functor
  };

	/// Base class of all polymorphic function object wrappers.
  class _Function_base
  {
  public:
    static const size_t _M_max_size = sizeof(_Nocopy_types);
    static const size_t _M_max_align = __alignof__(_Nocopy_types);

    template<typename _Functor>
      class _Base_manager
      {
      protected:
	static const bool __stored_locally =
	(__is_location_invariant<_Functor>::value
	 && sizeof(_Functor) <= _M_max_size
	 && __alignof__(_Functor) <= _M_max_align
	 && (_M_max_align % __alignof__(_Functor) == 0));

	using _Local_storage = integral_constant<bool, __stored_locally>;

	// Retrieve a pointer to the function object
	static _Functor*
	_M_get_pointer(const _Any_data& __source)
	{
	  if _GLIBCXX17_CONSTEXPR (__stored_locally)
	    {
	      const _Functor& __f = __source._M_access<_Functor>();
	      return const_cast<_Functor*>(std::__addressof(__f));
	    }
	  else // have stored a pointer
	    return __source._M_access<_Functor*>();
	}

      private:
	// Construct a location-invariant function object that fits within
	// an _Any_data structure.
	template<typename _Fn>
	  static void
	  _M_create(_Any_data& __dest, _Fn&& __f, true_type)
	  {
	    ::new (__dest._M_access()) _Functor(std::forward<_Fn>(__f));
	  }

	// Construct a function object on the heap and store a pointer.
	template<typename _Fn>
	  static void
	  _M_create(_Any_data& __dest, _Fn&& __f, false_type)
	  {
	    __dest._M_access<_Functor*>()
	      = new _Functor(std::forward<_Fn>(__f));
	  }

	// Destroy an object stored in the internal buffer.
	static void
	_M_destroy(_Any_data& __victim, true_type)
	{
	  __victim._M_access<_Functor>().~_Functor();
	}

	// Destroy an object located on the heap.
	static void
	_M_destroy(_Any_data& __victim, false_type)
	{
	  delete __victim._M_access<_Functor*>();
	}

      public:
	static bool
	_M_manager(_Any_data& __dest, const _Any_data& __source,
		   _Manager_operation __op)
	{
	  switch (__op)
	    {
// 	    case __get_type_info:
// #if __cpp_rtti
// 	      __dest._M_access<const type_info*>() = &typeid(_Functor);
// #else
// 	      __dest._M_access<const type_info*>() = nullptr;
// #endif
// 	      break;

	    case __get_functor_ptr:
	      __dest._M_access<_Functor*>() = _M_get_pointer(__source);
	      break;

	    case __clone_functor:
	      _M_init_functor(__dest,
		  *const_cast<const _Functor*>(_M_get_pointer(__source)));
	      break;

	    case __destroy_functor:
	      _M_destroy(__dest, _Local_storage());
	      break;
	    }
	  return false;
	}

	template<typename _Fn>
	  static void
	  _M_init_functor(_Any_data& __functor, _Fn&& __f)
	  noexcept(__and_<_Local_storage,
			  is_nothrow_constructible<_Functor, _Fn>>::value)
	  {
	    _M_create(__functor, std::forward<_Fn>(__f), _Local_storage());
	  }

	template<typename _Signature>
	  static bool
	  _M_not_empty_function(const function<_Signature>& __f)
	  { return static_cast<bool>(__f); }

	template<typename _Tp>
	  static bool
	  _M_not_empty_function(_Tp* __fp)
	  { return __fp != nullptr; }

	template<typename _Class, typename _Tp>
	  static bool
	  _M_not_empty_function(_Tp _Class::* __mp)
	  { return __mp != nullptr; }

	template<typename _Tp>
	  static bool
	  _M_not_empty_function(const _Tp&)
	  { return true; }
      };

    _Function_base() = default;

    ~_Function_base()
    {
      if (_M_manager)
	_M_manager(_M_functor, _M_functor, __destroy_functor);
    }

    bool _M_empty() const { return !_M_manager; }

    using _Manager_type
      = bool (*)(_Any_data&, const _Any_data&, _Manager_operation);

    _Any_data     _M_functor{};
    _Manager_type _M_manager{};
  };

	template<typename _Signature, typename _Functor>
    class _Function_handler;

  template<typename _Res, typename _Functor, typename... _ArgTypes>
    class _Function_handler<_Res(_ArgTypes...), _Functor>
    : public _Function_base::_Base_manager<_Functor>
    {
      using _Base = _Function_base::_Base_manager<_Functor>;

    public:
      static bool
      _M_manager(_Any_data& __dest, const _Any_data& __source,
		 _Manager_operation __op)
      {
	switch (__op)
	  {
// #if __cpp_rtti
// 	  case __get_type_info:
// 	    __dest._M_access<const type_info*>() = &typeid(_Functor);
// 	    break;
// #endif
	  case __get_functor_ptr:
	    __dest._M_access<_Functor*>() = _Base::_M_get_pointer(__source);
	    break;

	  default:
	    _Base::_M_manager(__dest, __source, __op);
	  }
	return false;
      }

      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes&&... __args)
      {
	return std::__invoke_r<_Res>(*_Base::_M_get_pointer(__functor),
				     std::forward<_ArgTypes>(__args)...);
      }

      template<typename _Fn>
	static constexpr bool
	_S_nothrow_init() noexcept
	{
	  return __and_<typename _Base::_Local_storage,
			is_nothrow_constructible<_Functor, _Fn>>::value;
	}
    };

  // Specialization for invalid types
  template<>
    class _Function_handler<void, void>
    {
    public:
      static bool
      _M_manager(_Any_data&, const _Any_data&, _Manager_operation)
      { return false; }
    };

	// Avoids instantiating ill-formed specializations of _Function_handler
  // in std::function<_Signature>::target<_Functor>().
  // e.g. _Function_handler<Sig, void()> and _Function_handler<Sig, void>
  // would be ill-formed.
  template<typename _Signature, typename _Functor,
	   bool __valid = is_object<_Functor>::value>
    struct _Target_handler
    : _Function_handler<_Signature, typename remove_cv<_Functor>::type>
    { };

  template<typename _Signature, typename _Functor>
    struct _Target_handler<_Signature, _Functor, false>
    : _Function_handler<void, void>
    { };


/**
   *  @brief Polymorphic function wrapper.
   *  @ingroup functors
   *  @since C++11
   */
  template<typename _Res, typename... _ArgTypes>
    class function<_Res(_ArgTypes...)>
    : public _Maybe_unary_or_binary_function<_Res, _ArgTypes...>,
      private _Function_base
    {
      // Equivalent to std::decay_t except that it produces an invalid type
      // if the decayed type is the current specialization of std::function.
      template<typename _Func,
	       bool _Self = is_same<__remove_cvref_t<_Func>, function>::value>
	using _Decay_t
	  = typename __enable_if_t<!_Self, decay<_Func>>::type;

      template<typename _Func,
	       typename _DFunc = _Decay_t<_Func>,
	       typename _Res2 = __invoke_result<_DFunc&, _ArgTypes...>>
	struct _Callable
	: __is_invocable_impl<_Res2, _Res>::type
	{ };

      template<typename _Cond, typename _Tp = void>
	using _Requires = __enable_if_t<_Cond::value, _Tp>;

      template<typename _Functor>
	using _Handler
	  = _Function_handler<_Res(_ArgTypes...), __decay_t<_Functor>>;

    public:
      typedef _Res result_type;

      // [3.7.2.1] construct/copy/destroy

      /**
       *  @brief Default construct creates an empty function call wrapper.
       *  @post `!(bool)*this`
       */
      function() noexcept
      : _Function_base() { }

      /**
       *  @brief Creates an empty function call wrapper.
       *  @post @c !(bool)*this
       */
      function(nullptr_t) noexcept
      : _Function_base() { }

      /**
       *  @brief %Function copy constructor.
       *  @param __x A %function object with identical call signature.
       *  @post `bool(*this) == bool(__x)`
       *
       *  The newly-created %function contains a copy of the target of
       *  `__x` (if it has one).
       */
      function(const function& __x)
      : _Function_base()
      {
	if (static_cast<bool>(__x))
	  {
	    __x._M_manager(_M_functor, __x._M_functor, __clone_functor);
	    _M_invoker = __x._M_invoker;
	    _M_manager = __x._M_manager;
	  }
      }

      /**
       *  @brief %Function move constructor.
       *  @param __x A %function object rvalue with identical call signature.
       *
       *  The newly-created %function contains the target of `__x`
       *  (if it has one).
       */
      function(function&& __x) noexcept
      : _Function_base(), _M_invoker(__x._M_invoker)
      {
	if (static_cast<bool>(__x))
	  {
	    _M_functor = __x._M_functor;
	    _M_manager = __x._M_manager;
	    __x._M_manager = nullptr;
	    __x._M_invoker = nullptr;
	  }
      }

      /**
       *  @brief Builds a %function that targets a copy of the incoming
       *  function object.
       *  @param __f A %function object that is callable with parameters of
       *  type `ArgTypes...` and returns a value convertible to `Res`.
       *
       *  The newly-created %function object will target a copy of
       *  `__f`. If `__f` is `reference_wrapper<F>`, then this function
       *  object will contain a reference to the function object `__f.get()`.
       *  If `__f` is a null function pointer, null pointer-to-member, or
       *  empty `std::function`, the newly-created object will be empty.
       *
       *  If `__f` is a non-null function pointer or an object of type
       *  `reference_wrapper<F>`, this function will not throw.
       */
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 2774. std::function construction vs assignment
      template<typename _Functor,
	       typename _Constraints = _Requires<_Callable<_Functor>>>
	function(_Functor&& __f)
	noexcept(_Handler<_Functor>::template _S_nothrow_init<_Functor>())
	: _Function_base()
	{
	  static_assert(is_copy_constructible<__decay_t<_Functor>>::value,
	      "std::function target must be copy-constructible");
	  static_assert(is_constructible<__decay_t<_Functor>, _Functor>::value,
	      "std::function target must be constructible from the "
	      "constructor argument");

	  using _My_handler = _Handler<_Functor>;

	  if (_My_handler::_M_not_empty_function(__f))
	    {
	      _My_handler::_M_init_functor(_M_functor,
					   std::forward<_Functor>(__f));
	      _M_invoker = &_My_handler::_M_invoke;
	      _M_manager = &_My_handler::_M_manager;
	    }
	}

      /**
       *  @brief %Function assignment operator.
       *  @param __x A %function with identical call signature.
       *  @post @c (bool)*this == (bool)x
       *  @returns @c *this
       *
       *  The target of @a __x is copied to @c *this. If @a __x has no
       *  target, then @c *this will be empty.
       *
       *  If @a __x targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      function&
      operator=(const function& __x)
      {
	function(__x).swap(*this);
	return *this;
      }

      /**
       *  @brief %Function move-assignment operator.
       *  @param __x A %function rvalue with identical call signature.
       *  @returns @c *this
       *
       *  The target of @a __x is moved to @c *this. If @a __x has no
       *  target, then @c *this will be empty.
       *
       *  If @a __x targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      function&
      operator=(function&& __x) noexcept
      {
	function(std::move(__x)).swap(*this);
	return *this;
      }

      /**
       *  @brief %Function assignment to zero.
       *  @post @c !(bool)*this
       *  @returns @c *this
       *
       *  The target of @c *this is deallocated, leaving it empty.
       */
      function&
      operator=(nullptr_t) noexcept
      {
	if (_M_manager)
	  {
	    _M_manager(_M_functor, _M_functor, __destroy_functor);
	    _M_manager = nullptr;
	    _M_invoker = nullptr;
	  }
	return *this;
      }

      /**
       *  @brief %Function assignment to a new target.
       *  @param __f A %function object that is callable with parameters of
       *  type @c T1, @c T2, ..., @c TN and returns a value convertible
       *  to @c Res.
       *  @return @c *this
       *
       *  This  %function object wrapper will target a copy of @a
       *  __f. If @a __f is @c reference_wrapper<F>, then this function
       *  object will contain a reference to the function object @c
       *  __f.get(). If @a __f is a NULL function pointer or NULL
       *  pointer-to-member, @c this object will be empty.
       *
       *  If @a __f is a non-NULL function pointer or an object of type @c
       *  reference_wrapper<F>, this function will not throw.
       */
      template<typename _Functor>
	_Requires<_Callable<_Functor>, function&>
	operator=(_Functor&& __f)
	noexcept(_Handler<_Functor>::template _S_nothrow_init<_Functor>())
	{
	  function(std::forward<_Functor>(__f)).swap(*this);
	  return *this;
	}

      /// @overload
      template<typename _Functor>
	function&
	operator=(reference_wrapper<_Functor> __f) noexcept
	{
	  function(__f).swap(*this);
	  return *this;
	}

      // [3.7.2.2] function modifiers

      /**
       *  @brief Swap the targets of two %function objects.
       *  @param __x A %function with identical call signature.
       *
       *  Swap the targets of @c this function object and @a __f. This
       *  function will not throw an %exception.
       */
      void swap(function& __x) noexcept
      {
	std::swap(_M_functor, __x._M_functor);
	std::swap(_M_manager, __x._M_manager);
	std::swap(_M_invoker, __x._M_invoker);
      }

      // [3.7.2.3] function capacity

      /**
       *  @brief Determine if the %function wrapper has a target.
       *
       *  @return @c true when this %function object contains a target,
       *  or @c false when it is empty.
       *
       *  This function will not throw an %exception.
       */
      explicit operator bool() const noexcept
      { return !_M_empty(); }

      // [3.7.2.4] function invocation

      /**
       *  @brief Invokes the function targeted by @c *this.
       *  @returns the result of the target.
       *  @throws bad_function_call when @c !(bool)*this
       *
       *  The function call operator invokes the target function object
       *  stored by @c this.
       */
      _Res
      operator()(_ArgTypes... __args) const
      {
	if (_M_empty()) {
		__throw_bad_function_call();
		// TODO: throw an exception or panic or do something else
	}
	return _M_invoker(_M_functor, std::forward<_ArgTypes>(__args)...);
      }

// #if __cpp_rtti
//       // [3.7.2.5] function target access
//       /**
//        *  @brief Determine the type of the target of this function object
//        *  wrapper.
//        *
//        *  @returns the type identifier of the target function object, or
//        *  @c typeid(void) if @c !(bool)*this.
//        *
//        *  This function will not throw an %exception.
//        */
//       const type_info&
//       target_type() const noexcept
//       {
// 	if (_M_manager)
// 	  {
// 	    _Any_data __typeinfo_result;
// 	    _M_manager(__typeinfo_result, _M_functor, __get_type_info);
// 	    if (auto __ti =  __typeinfo_result._M_access<const type_info*>())
// 	      return *__ti;
// 	  }
// 	return typeid(void);
//       }
// #endif

      /**
       *  @brief Access the stored target function object.
       *
       *  @return Returns a pointer to the stored target function object,
       *  if @c typeid(_Functor).equals(target_type()); otherwise, a null
       *  pointer.
       *
       * This function does not throw exceptions.
       *
       * @{
       */
      template<typename _Functor>
	_Functor*
	target() noexcept
	{
	  const function* __const_this = this;
	  const _Functor* __func = __const_this->template target<_Functor>();
	  // If is_function_v<_Functor> is true then const_cast<_Functor*>
	  // would be ill-formed, so use *const_cast<_Functor**> instead.
	  return *const_cast<_Functor**>(&__func);
	}

      template<typename _Functor>
	const _Functor*
	target() const noexcept
	{
	  if _GLIBCXX17_CONSTEXPR (is_object<_Functor>::value)
	    {
	      // For C++11 and C++14 if-constexpr is not used above, so
	      // _Target_handler avoids ill-formed _Function_handler types.
	      using _Handler = _Target_handler<_Res(_ArgTypes...), _Functor>;

	      if (_M_manager == &_Handler::_M_manager
// #if __cpp_rtti
// 		  || (_M_manager && typeid(_Functor) == target_type())
// #endif
		 )
		{
		  _Any_data __ptr;
		  _M_manager(__ptr, _M_functor, __get_functor_ptr);
		  return __ptr._M_access<const _Functor*>();
		}
	    }
	  return nullptr;
	}
      /// @}

    private:
      using _Invoker_type = _Res (*)(const _Any_data&, _ArgTypes&&...);
      _Invoker_type _M_invoker = nullptr;
    };
}
