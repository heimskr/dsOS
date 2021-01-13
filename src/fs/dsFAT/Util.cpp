#include "fs/dsFAT/Util.h"

namespace DsOS::FS::DsFAT::Util {
	std::optional<std::string> pathFirst(std::string path, std::string *remainder) {
		if (path.empty()) {
			if (remainder)
				*remainder = "";
			return {};
		}

		if (path.front() == '/')
			path = path.substr(1);

		size_t count, len = path.size();
		for (count = 0; count < len && path[count] != '/'; ++count);

		std::string out = path.substr(0, count);

		if (remainder) {
			std::string last = path.substr(count);
			*remainder = (count == path.size() || last == "/")? "" : last;
		}

		return {out};
	}
}
