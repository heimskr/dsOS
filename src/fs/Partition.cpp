#include "device/Device.h"
#include "fs/Partition.h"
#include "lib/printf.h"
#include "Kernel.h"

namespace DsOS::FS {
	int Partition::read(void *buffer, size_t size, off_t offset) {
		printf("\e[32m[read(buffer, %lu, %ld)]\e[0m\n", size, offset);
		return parent->read(buffer, size, offset);
	}

	int Partition::write(const void *buffer, size_t size, off_t offset) {
		printf("\e[32m[\e[31mwrite\e[32m(buffer, %lu, %ld)]\e[0m", size, offset);
		if (size == 320) {
			printf(":");
			for (size_t i = 0; i < size; ++i)
				printf(" %x", ((char *) buffer)[i] & 0xff);
			printf("\n");
		} else
			printf("\n");
		return parent->write(buffer, size, offset);
	}

	int Partition::clear() {
		return parent->clear(offset, length);
	}
}
