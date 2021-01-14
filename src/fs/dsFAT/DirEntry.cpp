#include "fs/dsFAT/Types.h"
#include "lib/printf.h"

namespace DsOS::FS::DsFAT {
	void DirEntry::reset() {
		memset(name.str, 0, sizeof(name));
		times = {0, 0, 0};
		length = 0;
		startBlock = -1;
		type = FileType::File;
		modes = 0;
	}

	void DirEntry::print() const {
		printf("DirEntry[name=\"%s\", length=%lu, startBlock=%d, type=%d, modes=%u]\n",
			name.str, length, startBlock, type, modes);
	}
}
