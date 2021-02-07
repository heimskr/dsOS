#include <cstdlib>

#include "ThornUtil.h"

namespace Thorn::Util {
	std::vector<std::string> split(const std::string &str, const std::string &delimiter, bool condense) {
		if (str.empty())
			return {};

		size_t next = str.find(delimiter);
		if (next == std::string::npos)
			return {str};

		std::vector<std::string> out {};
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
}
