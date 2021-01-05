// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_POINTER_TRAITS_H
#define _LIBCPP___MEMORY_POINTER_TRAITS_H

#include "lib/type_traits"

#pragma GCC system_header

namespace std {
	template <typename T, typename = void>
	struct __has_element_type: false_type {};

	template <typename T>
	struct __has_element_type<T, typename __void_t<typename T::element_type>::type>: true_type {};

	template <typename Ptr, bool = __has_element_type<Ptr>::value>
	struct __pointer_traits_element_type;

	template <typename Ptr>
	struct __pointer_traits_element_type<Ptr, true> {
		typedef typename Ptr::element_type type;
	};

	template <template <typename, typename...> typename S, typename T, typename... Args>
	struct __pointer_traits_element_type<S<T, Args...>, true> {
		typedef typename S<T, Args...>::element_type type;
	};

	template <template <typename, typename...> typename S, typename T, typename... Args>
	struct __pointer_traits_element_type<S<T, Args...>, false> {
		typedef T type;
	};

	template <typename T, typename = void>
	struct __has_difference_type: false_type {};

	template <typename T>
	struct __has_difference_type<T, typename __void_t<typename T::difference_type>::type>: true_type {};

	template <typename Ptr, bool = __has_difference_type<Ptr>::value>
	struct __pointer_traits_difference_type {
		typedef ptrdiff_t type;
	};

	template <typename Ptr>
	struct __pointer_traits_difference_type<Ptr, true> {
		typedef typename Ptr::difference_type type;
	};

	template <typename T, typename U>
	struct __has_rebind {
		private:
			struct __two {char __lx; char __lxx;};

			template <typename _Xp>
			static __two __test(...);

			template <typename _Xp>
			static char __test(typename _Xp::template rebind<U>* = 0);

		public:
			static const bool value = sizeof(__test<T>(0)) == 1;
	};

	template <typename T, typename U, bool = __has_rebind<T, U>::value>
	struct __pointer_traits_rebind {
		typedef typename T::template rebind<U> type;
	};

	template <template <typename, typename...> typename S, typename T, typename ...Args, typename U>
	struct __pointer_traits_rebind<S<T, Args...>, U, true> {
		typedef typename S<T, Args...>::template rebind<U> type;
	};

	template <template <typename, typename...> typename S, typename T, typename ...Args, typename U>
	struct __pointer_traits_rebind<S<T, Args...>, U, false> {
		typedef S<U, Args...> type;
	};

	template <typename Ptr>
	struct pointer_traits {
		typedef Ptr pointer;
		typedef typename __pointer_traits_element_type<pointer>::type element_type;
		typedef typename __pointer_traits_difference_type<pointer>::type difference_type;

		template <typename U>
		using rebind = typename __pointer_traits_rebind<pointer, U>::type;

		private:
			struct __nat {};

		public:
			static pointer pointer_to(typename conditional<is_void<element_type>::value, __nat, element_type>::type &__r) {
				return pointer::pointer_to(__r);
			}
	};

	template <typename T>
	struct pointer_traits<T *> {
		typedef T * pointer;
		typedef T element_type;
		typedef ptrdiff_t difference_type;

		template <typename U> using rebind = U*;

		private:
			struct __nat {};
		
		public:
			constexpr static pointer pointer_to(typename conditional<is_void<element_type>::value, __nat, element_type>::type& __r) noexcept {
				return std::addressof(__r);
			}
	};

	template <typename From, typename To>
	struct __rebind_pointer {
		typedef typename pointer_traits<From>::template rebind<To> type;
	};
}

#endif
