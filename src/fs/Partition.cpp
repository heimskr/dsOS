#include "device/Device.h"
#include "fs/Partition.h"

namespace DsOS::FS {
	int Partition::read(void *buffer, size_t size, off_t offset) {
		return parent->read(buffer, size, offset);
	}

	int Partition::write(const void *buffer, size_t size, off_t offset) {
		return parent->write(buffer, size, offset);
	}
}
