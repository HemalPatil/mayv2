#pragma once

#include <utility.cpp>

namespace std {
	// default_delete, used by unique_ptr for single objects
	template <typename Type>
	class default_delete {
		public:
			constexpr default_delete() noexcept = default;

			template <typename UniquePointer, typename = _Require<is_convertible<UniquePointer *, Type *>>>
			default_delete(const default_delete<UniquePointer> &) noexcept {}

			void operator()(Type *pointer) const {
				static_assert(!is_void<Type>::value, "can't delete pointer to incomplete type");
				static_assert(sizeof(Type) > 0, "can't delete pointer to incomplete type");
				delete pointer;
			}
	};

	// default_delete for arrays, used by unique_ptr<T[]>
	template <typename Type>
	class default_delete<Type[]> {
		public:
			constexpr default_delete() noexcept = default;

			template <typename UniquePointer, typename = _Require<is_convertible<UniquePointer (*)[], Type (*)[]>>>
			default_delete(const default_delete<UniquePointer[]> &) noexcept {}

			template <typename UniquePointer>
			typename enable_if<is_convertible<UniquePointer (*)[], Type (*)[]>::value>::type
			operator()(UniquePointer *pointer) const {
				static_assert(sizeof(Type) > 0, "can't delete pointer to incomplete type");
				delete[] pointer;
			}
	};

	// Manages the pointer and deleter of a unique_ptr
	template <typename _Tp, typename _Dp>
	class __uniq_ptr_impl {
		private: // TODO: remove
			template <typename _Up, typename _Ep, typename = void>
			struct _Ptr { using type = _Up*; };

			template <typename _Up, typename _Ep>
			struct _Ptr<_Up, _Ep, void_t<typename remove_reference<_Ep>::type::pointer>> {
				using type = typename remove_reference<_Ep>::type::pointer;
			};

		public:
			using _DeleterConstraint = enable_if<__and_<__not_<is_pointer<_Dp>>, is_default_constructible<_Dp>>::value>;
			using pointer = typename _Ptr<_Tp, _Dp>::type;

			static_assert(
				!is_rvalue_reference<_Dp>::value,
				"unique_ptr's deleter type must be a function object type or an lvalue reference type" 
			);

			__uniq_ptr_impl() = default;
			__uniq_ptr_impl(pointer __p) : _M_p(), _M_Dp() { _M_ptr() = __p; }

			template<typename _Del>
			__uniq_ptr_impl(pointer __p, _Del&& __d) : _M_p(__p), _M_Dp(std::forward<_Del>(__d)) { }

			__uniq_ptr_impl(__uniq_ptr_impl&& __u) noexcept : _M_p(std::move(__u._M_p)), _M_Dp(std::move(__u._M_Dp)) { __u._M_ptr() = nullptr; }

			__uniq_ptr_impl& operator=(__uniq_ptr_impl&& __u) noexcept {
				reset(__u.release());
				_M_deleter() = std::forward<_Dp>(__u._M_deleter());
				return *this;
			}

			pointer& _M_ptr() { return _M_p; }
			pointer _M_ptr() const { return _M_p; }
			_Dp& _M_deleter() { return _M_Dp; }
			const _Dp& _M_deleter() const { return _M_Dp; }

			void reset(pointer __p) noexcept {
				const pointer __old_p = _M_ptr();
				_M_ptr() = __p;
				if (__old_p) {
					_M_deleter()(__old_p);
				}
			}

			pointer release() noexcept {
				pointer __p = _M_ptr();
				_M_ptr() = nullptr;
				return __p;
			}

			void swap(__uniq_ptr_impl& __rhs) noexcept {
				std::swap(this->_M_ptr(), __rhs._M_ptr());
				std::swap(this->_M_deleter(), __rhs._M_deleter());
			}

		private:
			pointer _M_p;
			_Dp _M_Dp;
	};

	// Defines move construction + assignment as either defaulted or deleted.
	template <typename _Tp, typename _Dp, bool = is_move_constructible<_Dp>::value, bool = is_move_assignable<_Dp>::value>
	struct UniquePointerData : __uniq_ptr_impl<_Tp, _Dp> {
		using __uniq_ptr_impl<_Tp, _Dp>::__uniq_ptr_impl;
		UniquePointerData(UniquePointerData&&) = default;
		UniquePointerData& operator=(UniquePointerData&&) = default;
	};

	template <typename _Tp, typename _Dp>
	struct UniquePointerData<_Tp, _Dp, true, false> : __uniq_ptr_impl<_Tp, _Dp> {
		using __uniq_ptr_impl<_Tp, _Dp>::__uniq_ptr_impl;
		UniquePointerData(UniquePointerData&&) = default;
		UniquePointerData& operator=(UniquePointerData&&) = delete;
	};

	template <typename _Tp, typename _Dp>
	struct UniquePointerData<_Tp, _Dp, false, true> : __uniq_ptr_impl<_Tp, _Dp> {
		using __uniq_ptr_impl<_Tp, _Dp>::__uniq_ptr_impl;
		UniquePointerData(UniquePointerData&&) = delete;
		UniquePointerData& operator=(UniquePointerData&&) = default;
	};

	template <typename _Tp, typename _Dp>
	struct UniquePointerData<_Tp, _Dp, false, false> : __uniq_ptr_impl<_Tp, _Dp> {
		using __uniq_ptr_impl<_Tp, _Dp>::__uniq_ptr_impl;
		UniquePointerData(UniquePointerData&&) = delete;
		UniquePointerData& operator=(UniquePointerData&&) = delete;
	};

	// unique_ptr for single objects.
	template <typename _Tp, typename _Dp = default_delete<_Tp>>
	class unique_ptr {
		private:
			template <typename _Up>
			using _DeleterConstraint = typename __uniq_ptr_impl<_Tp, _Up>::_DeleterConstraint::type;

			UniquePointerData<_Tp, _Dp> _M_t;

		public:
			using pointer	  = typename __uniq_ptr_impl<_Tp, _Dp>::pointer;
			using element_type  = _Tp;
			using deleter_type  = _Dp;

		private:
			// helper template for detecting a safe conversion from another unique_ptr
			template<typename _Up, typename _Ep>
			using __safe_conversion_up = __and_<is_convertible<typename unique_ptr<_Up, _Ep>::pointer, pointer>,__not_<is_array<_Up>>>;

		public:
			// Constructors.

      /// Default constructor, creates a unique_ptr that owns nothing.
      template<typename _Del = _Dp, typename = _DeleterConstraint<_Del>>
	constexpr unique_ptr() noexcept
	: _M_t()
	{ }

      /** Takes ownership of a pointer.
       *
       * @param __p  A pointer to an object of @c element_type
       *
       * The deleter will be value-initialized.
       */
      template<typename _Del = _Dp, typename = _DeleterConstraint<_Del>>
	explicit
	unique_ptr(pointer __p) noexcept
	: _M_t(__p)
        { }

      /** Takes ownership of a pointer.
       *
       * @param __p  A pointer to an object of @c element_type
       * @param __d  A reference to a deleter.
       *
       * The deleter will be initialized with @p __d
       */
      template<typename _Del = deleter_type,
	       typename = _Require<is_copy_constructible<_Del>>>
	unique_ptr(pointer __p, const deleter_type& __d) noexcept
	: _M_t(__p, __d) { }

      /** Takes ownership of a pointer.
       *
       * @param __p  A pointer to an object of @c element_type
       * @param __d  An rvalue reference to a (non-reference) deleter.
       *
       * The deleter will be initialized with @p std::move(__d)
       */
      template<typename _Del = deleter_type,
	       typename = _Require<is_move_constructible<_Del>>>
	unique_ptr(pointer __p,
		   enable_if_t<!is_lvalue_reference<_Del>::value,
				 _Del&&> __d) noexcept
	: _M_t(__p, std::move(__d))
	{ }

      template<typename _Del = deleter_type,
	       typename _DelUnref = typename remove_reference<_Del>::type>
	unique_ptr(pointer,
		   enable_if_t<is_lvalue_reference<_Del>::value,
				 _DelUnref&&>) = delete;

      /// Creates a unique_ptr that owns nothing.
      template<typename _Del = _Dp, typename = _DeleterConstraint<_Del>>
	constexpr unique_ptr(nullptr_t) noexcept
	: _M_t()
	{ }

      // Move constructors.

      /// Move constructor.
      unique_ptr(unique_ptr&&) = default;

      /** @brief Converting constructor from another type
       *
       * Requires that the pointer owned by @p __u is convertible to the
       * type of pointer owned by this object, @p __u does not own an array,
       * and @p __u has a compatible deleter type.
       */
      template<typename _Up, typename _Ep, typename = _Require<
               __safe_conversion_up<_Up, _Ep>,
	       typename conditional<is_reference<_Dp>::value,
				    is_same<_Ep, _Dp>,
				    is_convertible<_Ep, _Dp>>::type>>
	unique_ptr(unique_ptr<_Up, _Ep>&& __u) noexcept
	: _M_t(__u.release(), std::forward<_Ep>(__u.get_deleter()))
	{ }

#if _GLIBCXX_USE_DEPRECATED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      /// Converting constructor from @c auto_ptr
      template<typename _Up, typename = _Require<
	       is_convertible<_Up*, _Tp*>, is_same<_Dp, default_delete<_Tp>>>>
	unique_ptr(auto_ptr<_Up>&& __u) noexcept;
#pragma GCC diagnostic pop
#endif

      /// Destructor, invokes the deleter if the stored pointer is not null.
      ~unique_ptr() noexcept
      {
	// static_assert(is_invocable<deleter_type&, pointer>::value,
	// 	      "unique_ptr's deleter must be invocable with a pointer");
	auto& __ptr = _M_t._M_ptr();
	if (__ptr != nullptr)
	  get_deleter()(std::move(__ptr));
	__ptr = pointer();
      }

      // Assignment.

      /** @brief Move assignment operator.
       *
       * Invokes the deleter if this object owns a pointer.
       */
      unique_ptr& operator=(unique_ptr&&) = default;

      /** @brief Assignment from another type.
       *
       * @param __u  The object to transfer ownership from, which owns a
       *             convertible pointer to a non-array object.
       *
       * Invokes the deleter if this object owns a pointer.
       */
      template<typename _Up, typename _Ep>
        typename enable_if< __and_<
          __safe_conversion_up<_Up, _Ep>,
          is_assignable<deleter_type&, _Ep&&>
          >::value,
          unique_ptr&>::type
	operator=(unique_ptr<_Up, _Ep>&& __u) noexcept
	{
	  reset(__u.release());
	  get_deleter() = std::forward<_Ep>(__u.get_deleter());
	  return *this;
	}

      /// Reset the %unique_ptr to empty, invoking the deleter if necessary.
      unique_ptr&
      operator=(nullptr_t) noexcept
      {
	reset();
	return *this;
      }

      // Observers.

      /// Dereference the stored pointer.
      typename add_lvalue_reference<element_type>::type
      operator*() const
      {
	__glibcxx_assert(get() != pointer());
	return *get();
      }

      /// Return the stored pointer.
      pointer
      operator->() const noexcept
      {
	_GLIBCXX_DEBUG_PEDASSERT(get() != pointer());
	return get();
      }

      /// Return the stored pointer.
      pointer
      get() const noexcept
      { return _M_t._M_ptr(); }

      /// Return a reference to the stored deleter.
      deleter_type&
      get_deleter() noexcept
      { return _M_t._M_deleter(); }

      /// Return a reference to the stored deleter.
      const deleter_type&
      get_deleter() const noexcept
      { return _M_t._M_deleter(); }

      /// Return @c true if the stored pointer is not null.
      explicit operator bool() const noexcept
      { return get() == pointer() ? false : true; }

      // Modifiers.

      /// Release ownership of any stored pointer.
      pointer
      release() noexcept
      { return _M_t.release(); }

      /** @brief Replace the stored pointer.
       *
       * @param __p  The new pointer to store.
       *
       * The deleter will be invoked if a pointer is already owned.
       */
      void
      reset(pointer __p = pointer()) noexcept
      {
	// static_assert(is_invocable<deleter_type&, pointer>::value,
	// 	      "unique_ptr's deleter must be invocable with a pointer");
	_M_t.reset(std::move(__p));
      }

      /// Exchange the pointer and deleter with another object.
      void
      swap(unique_ptr& __u) noexcept
      {
	// static_assert(__is_swappable<_Dp>::value, "deleter must be swappable");
	_M_t.swap(__u._M_t);
      }

      // Disable copy from lvalue.
      unique_ptr(const unique_ptr&) = delete;
      unique_ptr& operator=(const unique_ptr&) = delete;
  };

	template <typename Type>
	struct MakeUniqueType
	{
		typedef unique_ptr<Type> singleObject;
	};

	template <typename Type>
	struct MakeUniqueType<Type[]>
	{
		typedef unique_ptr<Type[]> arrayObject;
	};

	template <typename Type, size_t Bound>
	struct MakeUniqueType<Type[Bound]>
	{
		struct InvalidType
		{
		};
	};

	// make_unique for single objects
	template <typename Type, typename... Args>
	inline typename MakeUniqueType<Type>::singleObject
	make_unique(Args &&...args)
	{
		return unique_ptr<Type>(new Type(std::forward<Args>(args)...));
	}

	// make_unique for arrays of unknown bound
	template <typename Type>
	inline typename MakeUniqueType<Type>::arrayObject
	make_unique(size_t count)
	{
		return unique_ptr<Type>(new remove_extent_t<Type>[count]());
	}

	// Disable make_unique for arrays of known bound
	template <typename Type, typename... Args>
	typename MakeUniqueType<Type>::InvalidType
	make_unique(Args &&...) = delete;
}
