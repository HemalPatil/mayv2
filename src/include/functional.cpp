#pragma once

#include <memory.cpp>

namespace std {
	template <typename T>
	class function;

	template <typename ReturnValue, typename... Args>
	class function<ReturnValue(Args...)> {
	public:
		template <typename T>
		function& operator=(T t) {
			callable_ = std::make_unique<CallableT<T>>(t);
			// std::cout << "cons" << std::endl;
			return *this;
		}

		ReturnValue operator()(Args... args) const {
			// std::cout << "op()" <<std::endl;
			return callable_->Invoke(args...);
		}

	private:
		class ICallable {
		public:
			virtual ~ICallable() = default;
			virtual ReturnValue Invoke(Args...) = 0;
		};

		template <typename T>
		class CallableT : public ICallable {
		public:
			CallableT(const T& t)
				: t_(t) {
			}

			~CallableT() override = default;

			ReturnValue Invoke(Args... args) override {
				// std::cout << "fin" << std::endl;
				return t_(args...);
			}

		private:
			T t_;
		};

		std::unique_ptr<ICallable> callable_;
	};
}
