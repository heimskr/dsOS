#pragma once

#include <memory>
#include <vector>

namespace Thorn {
	struct StorageDeviceBase;
}

namespace Thorn::FS {

	struct Record {
		size_t size;
		size_t offset;

		Record(size_t size_, size_t offset_): size(size_), offset(offset_) {}
	};

	struct Partition {
		std::vector<Record> writeRecords, readRecords;
		StorageDeviceBase *parent;
		/** Number of bytes after the start of the disk. */
		size_t offset;
		/** Length of the partition in bytes. */
		size_t length;

		Partition(StorageDeviceBase *parent_, size_t offset_, size_t length_):
			parent(parent_), offset(offset_), length(length_) {}

		int read(void *buffer, size_t size, size_t byte_offset);
		int write(const void *buffer, size_t size, size_t byte_offset);
		int clear();
	};
}