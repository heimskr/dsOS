#include "fs/ThornFAT/Types.h"
#include "lib/printf.h"

namespace Thorn::FS::ThornFAT {
	void Superblock::print() const {
		printf("Superblock[magic=0x%x, blockCount=%lu, fatBlocks=%u, blockSize=%u, startBlock=%d]\n",
			magic, blockCount, fatBlocks, blockSize, startBlock);
	}
}
