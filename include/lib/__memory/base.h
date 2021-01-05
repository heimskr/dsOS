// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_BASE_H
#define _LIBCPP___MEMORY_BASE_H

#include "lib/printf.h"
#include "lib/type_traits"

#pragma GCC system_header

namespace std {
	// addressof

	template <typename T>
	inline T * addressof(T &__x) noexcept {
		return reinterpret_cast<T *>(const_cast<char *>(&reinterpret_cast<const volatile char &>(__x)));
	}

	template <typename T> T * addressof(const T &&) noexcept = delete;

	// construct_at

	template<typename T, typename ...Args, typename = decltype(::new (std::declval<void *>()) T(std::declval<Args>()...))>
	constexpr T * construct_at(T *location, Args && ...args) {
		if (!location) {
			printf("null pointer given to construct_at\n");
			for (;;);
		}
		return ::new ((void*)location) T(std::forward<Args>(args)...);
	}

	// destroy_at

	template <typename T>
	inline constexpr void destroy_at(T *loc) {
		loc->~T();
	}
}

#endif
