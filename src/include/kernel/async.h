#pragma once

#include <coroutine>
#include <functional>
#include <kernel.h>
#include <terminal.h>
#include <vector>

namespace Async {
	template<typename T>
	class ThenablePromise;

	template<typename T>
	class Thenable;

	template<typename T>
	class ThenablePromise {
		private:
			std::vector<std::coroutine_handle<>> awaitingCoroutines;
			bool done = false;
			T result;

		public:
			void addAwaitingCoroutine(std::coroutine_handle<> awaitingCoroutine) noexcept {
				this->awaitingCoroutines.push_back(awaitingCoroutine);
			}

			Thenable<T> get_return_object() noexcept {
				return Thenable<T>(std::coroutine_handle<ThenablePromise<T>>::from_promise(*this));
			}

			std::suspend_never initial_suspend() noexcept {
				return {};
			}

			std::suspend_never final_suspend() noexcept {
				for (auto awaitingCoroutine : this->awaitingCoroutines) {
					if (awaitingCoroutine && !awaitingCoroutine.done()) {
						awaitingCoroutine.resume();
					} else {
						terminalPrintString("\nAttempted to resume an invalid coroutine handle\n", 49);
						Kernel::panic();
					}
				}
				return {};
			}

			[[noreturn]] void unhandled_exception() noexcept {
				Kernel::panic();
			}

			bool isDone() noexcept {
				return this->done;
			}

			void return_value(const T &value) noexcept {
				this->result = value;
				this->done = true;
			}

			T& getResult() noexcept {
				if (!this->done) {
					terminalPrintString("\nAttempted to get result of unfinished coroutine\n", 49);
					Kernel::panic();
				}
				return this->result;
			}
	};

	template<>
	class ThenablePromise<void> {
		private:
			std::vector<std::coroutine_handle<>> awaitingCoroutines;
			bool done = false;

		public:
			void addAwaitingCoroutine(std::coroutine_handle<> awaitingCoroutine) noexcept {
				this->awaitingCoroutines.push_back(awaitingCoroutine);
			}

			Thenable<void> get_return_object() noexcept;

			std::suspend_never initial_suspend() noexcept {
				return {};
			}

			std::suspend_never final_suspend() noexcept {
				for (auto awaitingCoroutine : this->awaitingCoroutines) {
					if (awaitingCoroutine && !awaitingCoroutine.done()) {
						awaitingCoroutine.resume();
					} else {
						terminalPrintString("\nAttempted to resume an invalid coroutine handle\n", 49);
						Kernel::panic();
					}
				}
				return {};
			}

			[[noreturn]] void unhandled_exception() noexcept {
				Kernel::panic();
			}

			bool isDone() noexcept {
				return this->done;
			}

			void return_void() noexcept {
				this->done = true;
			}
	};

	template<typename T>
	class [[nodiscard]] Thenable {
		private:
			std::coroutine_handle<ThenablePromise<T>> coroutineHandle;

		public:
			using promise_type = ThenablePromise<T>;

			Thenable() : coroutineHandle(nullptr) {}

			explicit Thenable(std::coroutine_handle<ThenablePromise<T>> coroutineHandle) noexcept : coroutineHandle(coroutineHandle) {}

			Thenable(Thenable &&other) noexcept : coroutineHandle(other.coroutineHandle) {
				other.coroutineHandle = nullptr;
			}

			Thenable& operator=(Thenable &&other) noexcept {
				this->coroutineHandle = other.coroutineHandle;
				other.coroutineHandle = nullptr;
				return *this;
			}

			Thenable(const Thenable&) = delete;
			Thenable& operator=(const Thenable&) = delete;

			bool await_ready() const noexcept {
				return this->coroutineHandle.promise().isDone();
			}

			void await_suspend(std::coroutine_handle<> awaitingCoroutine) {
				this->coroutineHandle.promise().addAwaitingCoroutine(awaitingCoroutine);
			}

			T await_resume() const noexcept {
				return this->coroutineHandle.promise().getResult();
			}

			template<typename ThenT>
			Thenable<ThenT> then(std::function<ThenT()> func) & noexcept {
				co_await *this;
				co_return func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(std::function<ThenT()> func) && noexcept {
				Thenable<T> moveThis(std::move(*this));
				co_await moveThis;
				co_return func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(std::function<ThenT(T)> func) & noexcept {
				T result = co_await *this;
				co_return func(result);
			}

			template<typename ThenT>
			Thenable<ThenT> then(std::function<ThenT(T)> func) && noexcept {
				Thenable<T> moveThis(std::move(*this));
				T result = co_await moveThis;
				co_return func(result);
			}

			template<typename ThenT>
			Thenable<ThenT> then(Thenable<ThenT> (*func)()) & noexcept {
				co_await *this;
				co_return co_await func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(Thenable<ThenT> (*func)()) && noexcept {
				Thenable<T> moveThis(std::move(*this));
				co_await moveThis;
				co_return co_await func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(Thenable<ThenT> (*func)(T)) & noexcept {
				T result = co_await *this;
				co_return co_await func(result);
			}

			template<typename ThenT>
			Thenable<ThenT> then(Thenable<ThenT> (*func)(T)) && noexcept {
				Thenable<T> moveThis(std::move(*this));
				T result = co_await moveThis;
				co_return co_await func(result);
			}
	};

	template<>
	class [[nodiscard]] Thenable<void> {
		private:
			std::coroutine_handle<ThenablePromise<void>> coroutineHandle;

		public:
			using promise_type = ThenablePromise<void>;

			Thenable() : coroutineHandle(nullptr) {}

			explicit Thenable(std::coroutine_handle<ThenablePromise<void>> coroutineHandle) noexcept : coroutineHandle(coroutineHandle) {}

			Thenable(Thenable &&other) noexcept : coroutineHandle(other.coroutineHandle) {
				other.coroutineHandle = nullptr;
			}

			Thenable& operator=(Thenable &&other) noexcept {
				this->coroutineHandle = other.coroutineHandle;
				other.coroutineHandle = nullptr;
				return *this;
			}

			Thenable(const Thenable&) = delete;
			Thenable& operator=(const Thenable&) = delete;

			bool await_ready() const noexcept {
				return this->coroutineHandle.promise().isDone();
			}

			void await_suspend(std::coroutine_handle<> awaitingCoroutine) {
				this->coroutineHandle.promise().addAwaitingCoroutine(awaitingCoroutine);
			}

			void await_resume() const noexcept {}

			template<typename ThenT>
			Thenable<ThenT> then(std::function<ThenT()> func) & noexcept {
				co_await *this;
				co_return func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(std::function<ThenT()> func) && noexcept {
				Thenable<void> moveThis(std::move(*this));
				co_await moveThis;
				co_return func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(Thenable<ThenT> (*func)()) & noexcept {
				co_await *this;
				co_return co_await func();
			}

			template<typename ThenT>
			Thenable<ThenT> then(Thenable<ThenT> (*func)()) && noexcept {
				Thenable<void> moveThis(std::move(*this));
				co_await moveThis;
				co_return co_await func();
			}
	};

	inline Thenable<void> ThenablePromise<void>::get_return_object() noexcept {
		return Thenable<void>(std::coroutine_handle<ThenablePromise<void>>::from_promise(*this));
	}
}
