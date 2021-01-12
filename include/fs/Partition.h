#pragma once

#include <memory>

namespace DsOS::Device {
	struct DeviceBase;
}

namespace DsOS::FS {

	struct Partition {
		Device::DeviceBase *parent;
		off_t offset;
		size_t length;

		Partition(Device::DeviceBase *parent_, off_t offset_, size_t length_):
			parent(parent_), offset(offset_), length(length_) {}

		int read(void *buffer, size_t size, off_t offset = 0);
		int write(const void *buffer, size_t size, off_t offset = 0);
	};
}