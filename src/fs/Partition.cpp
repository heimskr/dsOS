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
		printf("\e[32m[\e[31mwrite\e[32m(buffer, %lu, %ld)]\e[0m\n", size, offset);
		if (size == 4 && offset == 4800) Kernel::backtrace();
		return parent->write(buffer, size, offset);
	}

	int Partition::clear() {
		return parent->clear(offset, length);
	}
}
