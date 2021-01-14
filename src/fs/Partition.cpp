#include "device/Device.h"
#include "fs/Partition.h"
#include "Kernel.h"

namespace DsOS::FS {
	int Partition::read(void *buffer, size_t size, off_t offset) {
		return parent->read(buffer, size, offset);
	}

	int Partition::write(const void *buffer, size_t size, off_t offset) {
		printf("Partition::write(buffer, %lu, 0x%lx : %lu)\n", size, offset, offset);
		if (size == 320 && offset == 0x1400) {
			Kernel::backtrace();
			for (size_t i = 0; i < size; ++i)
				printf("| [%lu] '%c' (%d)\n", i, ((char *) buffer)[i], ((char *) buffer)[i]);
		}
		int status = parent->write(buffer, size, offset);
		char *check = new char[size];
		parent->read(check, size, offset);
		size_t mistakes = 0;
		for (size_t i = 0; i < size; ++i) {
			if (check[i] != ((char *) buffer)[i]) {
				++mistakes;
				printf("Mismatch at %lu: '%c' (%d) instead of '%c' (%d)\n", i, check[i], check[i] & 0xff, ((char *) buffer)[i], ((char *) buffer)[i] & 0xff);
			}
		}
		printf("Mistakes: %lu\n", mistakes);
		// if (size == 320 && offset == 0x1400) for(;;);
		return status;
	}

	int Partition::clear() {
		printf("\e[31mPartition::clear(%ld, %lu)\e[0m\n", offset, length);
		return parent->clear(offset, length);
	}
}
