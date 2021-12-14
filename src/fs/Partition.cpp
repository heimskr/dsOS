#include "device/Device.h"
#include "fs/Partition.h"
#include "lib/printf.h"
#include "Kernel.h"

// #define DEBUG_WRITES
// #define VERIFY_WRITES
// #define VERIFY_WRITES_QUIETLY

namespace Thorn::FS {
	int Partition::read(void *buffer, size_t size, size_t byte_offset) {
		readRecords.emplace_back(size, offset);
		// printf("\e[32m[read(buffer, %lu, %ld)]\e[0m\n", size, offset);
		return parent->read(buffer, size, offset + byte_offset);
	}

	int Partition::write(const void *buffer, size_t size, size_t byte_offset) {
		writeRecords.emplace_back(size, byte_offset);
#ifdef DEBUG_WRITES
		printf("\e[32m[\e[31mwrite\e[32m(buffer, %lu, %ld)]\e[0m", size, byte_offset);
		if (size == 320) {
			printf(":");
			for (size_t i = 0; i < size; ++i)
				printf(" %x", ((char *) buffer)[i] & 0xff);
			printf("\n");
		} else
			printf("\n");
#endif
#ifndef VERIFY_WRITES
		return parent->write(buffer, size, offset + byte_offset);
#else
		int status = parent->write(buffer, size, offset + byte_offset);
		if (status != 0)
			return status;
		char *verify = new char[size];
		memset(verify, 0, size);
		status = parent->read(verify, size, offset + byte_offset);
		if (status != 0)
			return status;
		size_t mistakes = 0;
		for (size_t i = 0; i < size; ++i)
			if (((char *) buffer)[i] != verify[i]) {
				printf("Mistake at %lu: %02x should be %02x ('%c' vs. '%c')\n", i, verify[i] & 0xff,
					((char *) buffer)[i] & 0xff, verify[i] & 0xff, ((char *) buffer)[i] & 0xff);
				++mistakes;
			}
#ifdef VERIFY_WRITES_QUIETLY
		if (mistakes != 0)
#endif
		printf("Found %lu mistake%s%s (size=%lu, offset=0x%lx / %lu) memcmp = %d\n",
			mistakes, mistakes == 1? "" : "s", mistakes == 0? "." : "!!!", size, offset, offset,
			memcmp(buffer, verify, size));
		// if (mistakes == 0 && 4 < size) {
		// 	printf("Found no mistakes (size=%lu, offset=0x%lx / %lu).\n", size, offset, offset);
		// 	for (size_t i = 0; i < size; ++i)
		// 		printf("%lu: %02x is %02x.\n", i, verify[i] & 0xff, ((char *) buffer)[i] & 0xff);
		// }
		return 0;
#endif
	}

	int Partition::clear() {
		return parent->clear(offset, length);
	}
}
