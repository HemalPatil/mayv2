	// unique_ptr for single objects
	template <typename Type, typename DefaultDeleteType = default_delete<Type>>
	class unique_ptr {
		private: // TODO: remove
			template <typename _Up>
			using DeleterConstraint = typename __uniq_ptr_impl<Type, _Up>::DeleterConstraint::type;

			UniquePointerData<Type, DefaultDeleteType> _M_t;

		public:
			using pointer = typename __uniq_ptr_impl<Type, DefaultDeleteType>::pointer;
			using element_type = Type;
			using deleter_type = DefaultDeleteType;

		private:
			// helper template for detecting a safe conversion from another unique_ptr
			template <typename _Up, typename _Ep>
			using __safe_conversion_up = __and_<is_convertible<typename unique_ptr<_Up, _Ep>::pointer, pointer>, __not_<is_array<_Up>>>;

		public:
			// Constructors.

			// Default constructor, creates a unique_ptr that owns nothing.
			template <typename _Del = DefaultDeleteType, typename = DeleterConstraint<_Del>>
			constexpr unique_ptr() noexcept : _M_t() {}

			/** Takes ownership of a pointer.
			 *
			 * @param __p  A pointer to an object of @c element_type
			 *
			 * The deleter will be value-initialized.
			 */
			template <typename _Del = DefaultDeleteType, typename = DeleterConstraint<_Del>>
			explicit unique_ptr(pointer __p) noexcept : _M_t(__p) {}

			/** Takes ownership of a pointer.
			 *
			 * @param __p  A pointer to an object of @c element_type
			 * @param __d  A reference to a deleter.
			 *
			 * The deleter will be initialized with @p __d
			 */
			template <typename _Del = deleter_type, typename = _Require<is_copy_constructible<_Del>>>
			unique_ptr(pointer __p, const deleter_type &__d) noexcept : _M_t(__p, __d) {}

			/** Takes ownership of a pointer.
			 *
			 * @param __p  A pointer to an object of @c element_type
			 * @param __d  An rvalue reference to a (non-reference) deleter.
			 *
			 * The deleter will be initialized with @p std::move(__d)
			 */
			template <typename _Del = deleter_type, typename = _Require<is_move_constructible<_Del>>>
			unique_ptr(pointer __p, enable_if_t<!is_lvalue_reference<_Del>::value, _Del &&> __d) noexcept : _M_t(__p, std::move(__d)) {}

			template <typename _Del = deleter_type,
						typename _DelUnref = typename remove_reference<_Del>::type>
			unique_ptr(pointer,
						__enable_if_t<is_lvalue_reference<_Del>::value,
									_DelUnref &&>) = delete;

			/// Creates a unique_ptr that owns nothing.
			template <typename _Del = DefaultDeleteType, typename = DeleterConstraint<_Del>>
			constexpr unique_ptr(nullptr_t) noexcept
				: _M_t()
			{
			}

			// Move constructors.

			/// Move constructor.
			unique_ptr(unique_ptr &&) = default;

			/** @brief Converting constructor from another type
			 *
			 * Requires that the pointer owned by @p __u is convertible to the
			 * type of pointer owned by this object, @p __u does not own an array,
			 * and @p __u has a compatible deleter type.
			 */
			template <typename _Up, typename _Ep, typename = _Require<__safe_conversion_up<_Up, _Ep>, typename conditional<is_reference<DefaultDeleteType>::value, is_same<_Ep, DefaultDeleteType>, is_convertible<_Ep, DefaultDeleteType>>::type>>
			unique_ptr(unique_ptr<_Up, _Ep> &&__u) noexcept
				: _M_t(__u.release(), std::forward<_Ep>(__u.get_deleter()))
			{
			}

#if _GLIBCXX_USE_DEPRECATED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		/// Converting constructor from @c auto_ptr
		template <typename _Up, typename = _Require<
									is_convertible<_Up *, Type *>, is_same<DefaultDeleteType, default_delete<Type>>>>
		unique_ptr(auto_ptr<_Up> &&__u) noexcept;
#pragma GCC diagnostic pop
#endif

			/// Destructor, invokes the deleter if the stored pointer is not null.
			~unique_ptr() noexcept
			{
				static_assert(__is_invocable<deleter_type &, pointer>::value,
							"unique_ptr's deleter must be invocable with a pointer");
				auto &__ptr = _M_t._M_ptr();
				if (__ptr != nullptr)
					get_deleter()(std::move(__ptr));
				__ptr = pointer();
			}

			// Assignment.

			/** @brief Move assignment operator.
			 *
			 * Invokes the deleter if this object owns a pointer.
			 */
			unique_ptr &operator=(unique_ptr &&) = default;

		/** @brief Assignment from another type.
		 *
		 * @param __u  The object to transfer ownership from, which owns a
		 *             convertible pointer to a non-array object.
		 *
		 * Invokes the deleter if this object owns a pointer.
		 */
		template <typename _Up, typename _Ep>
		typename enable_if<__and_<
							   __safe_conversion_up<_Up, _Ep>,
							   is_assignable<deleter_type &, _Ep &&>>::value,
						   unique_ptr &>::type
		operator=(unique_ptr<_Up, _Ep> &&__u) noexcept
		{
			reset(__u.release());
			get_deleter() = std::forward<_Ep>(__u.get_deleter());
			return *this;
		}

		/// Reset the %unique_ptr to empty, invoking the deleter if necessary.
		unique_ptr &
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
		{
			return _M_t._M_ptr();
		}

		/// Return a reference to the stored deleter.
		deleter_type &
		get_deleter() noexcept
		{
			return _M_t._M_deleter();
		}

		/// Return a reference to the stored deleter.
		const deleter_type &
		get_deleter() const noexcept
		{
			return _M_t._M_deleter();
		}

		/// Return @c true if the stored pointer is not null.
		explicit operator bool() const noexcept
		{
			return get() == pointer() ? false : true;
		}

		// Modifiers.

		/// Release ownership of any stored pointer.
		pointer
		release() noexcept
		{
			return _M_t.release();
		}

		/** @brief Replace the stored pointer.
		 *
		 * @param __p  The new pointer to store.
		 *
		 * The deleter will be invoked if a pointer is already owned.
		 */
		void
		reset(pointer __p = pointer()) noexcept
		{
			static_assert(
				__is_invocable<deleter_type &, pointer>::value,
				"unique_ptr's deleter must be invocable with a pointer"
			);
			_M_t.reset(std::move(__p));
		}

		/// Exchange the pointer and deleter with another object.
		void
		swap(unique_ptr &__u) noexcept
		{
			static_assert(__is_swappable<DefaultDeleteType>::value, "deleter must be swappable");
			_M_t.swap(__u._M_t);
		}

		// Disable copy from lvalue.
		unique_ptr(const unique_ptr &) = delete;
		unique_ptr &operator=(const unique_ptr &) = delete;
	};

	template <typename T>
	class unique_ptr<T[]>
	{
	private:
		T *ptr = nullptr;

	public:
		unique_ptr()
		{
			this->ptr = nullptr;
		}

		unique_ptr(T *ptr)
		{
			this->ptr = ptr;
		}

		unique_ptr(unique_ptr &&moveObj)
		{
			if (this->ptr != nullptr)
			{
				delete[] this->ptr;
			}
			this->ptr = moveObj.ptr;
			moveObj.ptr = nullptr;
		}

		void operator=(unique_ptr &&moveObj)
		{
			if (this->ptr != nullptr)
			{
				delete[] this->ptr;
			}
			this->ptr = moveObj.ptr;
			moveObj.ptr = nullptr;
		}

		unique_ptr(const unique_ptr &ptr) = delete;
		unique_ptr &operator=(const unique_ptr &obj) = delete;

		T *operator->()
		{
			return this->ptr;
		}

		T &operator*()
		{
			return *(this->ptr);
		}

		T &operator[](int index)
		{
			// FIXME: doesn't check if index < 0 due to exceptions not being unavailable
			return this->ptr[index];
		}

		~unique_ptr()
		{
			if (this->ptr != nullptr)
			{
				delete[] this->ptr;
			}
		}
	};
