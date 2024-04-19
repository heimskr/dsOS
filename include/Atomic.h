#pragma once

#include <atomic>

namespace Thorn {
	template <typename T>
	struct Atomic: std::atomic<T> {
		using std::atomic<T>::atomic;
		using std::atomic<T>::operator=;
		using std::atomic<T>::operator T;

		Atomic(const Atomic &other):
			std::atomic<T>(other.load()) {}

		Atomic(Atomic &&other):
			std::atomic<T>(other.load()) {}

		Atomic & operator=(const Atomic &other) {
			std::atomic<T>::operator=(other.load());
			return *this;
		}

		Atomic & operator=(Atomic &&other) {
			std::atomic<T>::operator=(other.load());
			return *this;
		}
	};
}
