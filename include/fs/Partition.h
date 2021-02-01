#pragma once

#include <memory>

namespace Thorn::Device {
	struct DeviceBase;
}

namespace Thorn::FS {

	struct Partition {
		Device::DeviceBase *parent;
		/** Number of bytes after the start of the disk. */
		off_t offset;
		/** Length of the partition in bytes. */
		size_t length;

		Partition(Device::DeviceBase *parent_, off_t offset_, size_t length_):
			parent(parent_), offset(offset_), length(length_) {}

		int read(void *buffer, size_t size, off_t offset);
		int write(const void *buffer, size_t size, off_t offset);
		int clear();
	};
}