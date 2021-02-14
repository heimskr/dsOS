#include "device/Device.h"
#include "fs/Partition.h"
#include "lib/printf.h"
#include "Kernel.h"

// #define DEBUG_WRITES
#define VERIFY_WRITES
#define VERIFY_WRITES_QUIETLY

namespace Thorn::FS {
	int Partition::read(void *buffer, size_t size, off_t offset) {
		readRecords.emplace_back(size, offset);
		// printf("\e[32m[read(buffer, %lu, %ld)]\e[0m\n", size, offset);
		return parent->read(buffer, size, offset);
	}

	int Partition::write(const void *buffer, size_t size, off_t offset) {
		writeRecords.emplace_back(size, offset);
#ifdef DEBUG_WRITES
		printf("\e[32m[\e[31mwrite\e[32m(buffer, %lu, %ld)]\e[0m", size, offset);
		if (size == 320) {
			printf(":");
			for (size_t i = 0; i < size; ++i)
				printf(" %x", ((char *) buffer)[i] & 0xff);
			printf("\n");
		} else
			printf("\n");
#endif
#ifndef VERIFY_WRITES
		return parent->write(buffer, size, offset);
#else
		int status = parent->write(buffer, size, offset);
		if (status != 0)
			return status;
		char *verify = new char[size];
		memset(verify, 0, size);
		status = parent->read(verify, size, offset);
		if (status != 0)
			return status;
		size_t mistakes = 0;
		for (size_t i = 0; i < size; ++i)
			if (((char *) buffer)[i] != verify[i]) {
				printf("Mistake at %lu: %x should be %x\n", i, verify[i] & 0xff, ((char *) buffer)[i] & 0xff);
				++mistakes;
			}
#ifdef VERIFY_WRITES_QUIETLY
		if (mistakes != 0)
#endif
		printf("Found %lu mistake%s%s (size=%lu, offset=0x%lx) memcmp = %d\n",
			mistakes, mistakes == 1? "" : "s", mistakes == 0? "." : "!!!", size, offset,
			memcmp(buffer, verify, size));
		return 0;
#endif
	}

	int Partition::clear() {
		return parent->clear(offset, length);
	}
}
