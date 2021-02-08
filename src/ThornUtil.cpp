#include <cstdlib>

#include "ThornUtil.h"

namespace Thorn::Util {
	bool parseLong(const std::string &str, long &out, int base) {
		const char *c_str = str.c_str();
		char *end;
		long parsed = strtol(c_str, &end, base);
		if (c_str + str.length() != end)
			return false;
		out = parsed;
		return true;
	}

	bool parseLong(const std::string *str, long &out, int base) {
		return parseLong(*str, out, base);
	}

	bool parseLong(const char *str, long &out, int base) {
		return parseLong(std::string(str), out, base);
	}

	bool parseUlong(const std::string &str, unsigned long &out, int base) {
		const char *c_str = str.c_str();
		char *end;
		unsigned long parsed = strtoul(c_str, &end, base);
		if (c_str + str.length() != end)
			return false;
		out = parsed;
		return true;
	}

	bool parseUlong(const std::string *str, unsigned long &out, int base) {
		return parseUlong(*str, out, base);
	}

	bool parseUlong(const char *str, unsigned long &out, int base) {
		return parseUlong(std::string(str), out, base);
	}
}
