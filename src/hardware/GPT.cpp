#include "hardware/GPT.h"
#include "lib/printf.h"

namespace Thorn::GPT {
	void GUID::print(bool newline) {
		printf("%x-%x-%x-%x-%x%x%x%x%x%x%s", timeLow, timeMid, timeHigh | (version << 12), clock, node[0], node[1],
			node[2], node[3], node[4], node[5], newline? "\n" : "");
	}

	GUID::operator std::string() const {
		char str[37];
		snprintf(str, sizeof(str), "%x-%x-%x-%x-%x%x%x%x%x%x", timeLow, timeMid, timeHigh | (version << 12), clock,
			node[0], node[1], node[2], node[3], node[4], node[5]);
		return str;
	}

	GUID::operator bool() const {
		if (timeLow || timeMid || timeHigh || version || clock)
			return true;
		for (unsigned i = 0; i < sizeof(node) / sizeof(node[0]); ++i)
			if (node[i])
				return true;
		return false;
	}

	void PartitionEntry::printName(bool newline) {
		for (unsigned i = 0; i < 36; ++i) {
			if (!name[i])
				break;
			printf("%c", name[i] & 0xff);
		}
		if (newline)
			printf("\n");
	}
}
