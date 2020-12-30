#pragma once

namespace DsOS::Util {
	template <typename T>
	inline T upalign(T number, int alignment) {
		return number + ((alignment - (number % alignment)) % alignment);
	}

	template <typename T>
	inline T downalign(T number, int alignment) {
		return number - (number % alignment);
	}
}
