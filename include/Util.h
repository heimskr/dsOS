#pragma once

namespace DsOS::Util {
	template <typename T>
	inline T align(T number, int alignment) {
		return number + ((alignment - (number % alignment)) % alignment);
	}
}
