#include "Util.h"

namespace DsOS::Util {
	size_t strlen(const char *str) {
		size_t out = 0;
		while (str[out])
			++out;
		return out;
	}
}
