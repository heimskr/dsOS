#include "fs/dsFAT/Types.h"
#include "lib/printf.h"

namespace DsOS::FS::DsFAT {
	void DirEntry::reset() {
		memset(name.str, 0, sizeof(name));
		times = {0, 0, 0};
		length = 0;
		startBlock = INVALID;
		type = FileType::File;
		modes = 0;
	}

	DirEntry::operator std::string() const {
		return "DirEntry[name=" + std::string(name.str) + ", length=" + std::to_string(length) + ", startBlock=" +
			std::to_string(startBlock) + ", type=" + std::to_string((int) type) + ", modes=" + std::to_string(modes) +
			"]";
	}

	void DirEntry::print() const {
		printf("%s\n", std::string(*this).c_str());
	}
}
