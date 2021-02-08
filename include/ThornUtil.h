#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Thorn::Util {
	template <template <typename T> typename C = std::vector>
	C<std::string> split(const std::string &str, const std::string &delimiter, bool condense = true) {
		if (str.empty())
			return C<std::string>();

		size_t next = str.find(delimiter);
		if (next == std::string::npos)
			return C<std::string> {str};

		C<std::string> out {};
		const size_t delimiter_length = delimiter.size();
		size_t last = 0;

		out.push_back(str.substr(0, next));

		while (next != std::string::npos) {
			last = next;
			next = str.find(delimiter, last + delimiter_length);
			std::string sub = str.substr(last + delimiter_length, next - last - delimiter_length);
			if (!sub.empty() || !condense)
				out.push_back(std::move(sub));
		}

		return out;
	}

	bool parseLong(const std::string &, long &out, int base = 10);
	bool parseLong(const std::string *, long &out, int base = 10);
	bool parseLong(const char *, long &out, int base = 10);
	bool parseUlong(const std::string &, unsigned long &out, int base = 10);
	bool parseUlong(const std::string *, unsigned long &out, int base = 10);
	bool parseUlong(const char *, unsigned long &out, int base = 10);

	template <typename T>
	inline T upalign(T number, int alignment) {
		return number + ((alignment - (number % alignment)) % alignment);
	}

	template <typename T>
	inline T downalign(T number, int alignment) {
		return number - (number % alignment);
	}

	template <typename T>
	inline T updiv(T n, T d) {
		return n / d + (n % d? 1 : 0);
	}

	inline uintptr_t __attribute__((always_inline)) getReturnAddress() {
		uintptr_t return_address;
		asm volatile("mov 8(%%rbp), %0" : "=r"(return_address));
		return return_address;
	}

	inline bool isCanonical(uintptr_t address) {
		return address < 0x0000800000000000 || 0xffff800000000000 <= address;
	}

	inline bool isCanonical(void *address) {
		return isCanonical((uintptr_t) address);
	}
}
