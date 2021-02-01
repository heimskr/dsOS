#include "hardware/ATA.h"

#include <stddef.h>

namespace Thorn::ATA {
	void DeviceInfo::copyModel(char *str) {
		size_t i, index = 0;
		for (i = 0; i < sizeof(model) / sizeof(model[0]); ++i) {
			const char first = model[i] >> 8;
			if (!first)
				break;
			str[index++] = first;
			const char second = model[i] & 0xff;
			if (!second)
				break;
			str[index++] = second;
		}
		str[index] = '\0';
		for (i = index - 1; i != SIZE_MAX && str[i] == ' '; --i)
			str[i] = '\0';
	}
}
