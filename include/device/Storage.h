#pragma once

#include <cstdint>
#include <string>

#include "Cache.h"
#include "Defs.h"

namespace Thorn {
	template <size_t Size>
	struct StorageCacheEntry {
		uint8_t data[Size];
		bool flushed = false;

		StorageCacheEntry() = default;
	};

	template <size_t CacheLength, size_t BlockSize>
	struct StorageCache: TreeCache<uint64_t, StorageCacheEntry<BlockSize>> {
		StorageCache():
			TreeCache<uint64_t, StorageCacheEntry<BlockSize>>(CacheLength) {}
	};

	struct StorageDeviceBase {
		virtual ~StorageDeviceBase() = default;

		virtual int read(void *buffer, size_t size, size_t offset) = 0;
		virtual int write(const void *buffer, size_t size, size_t offset) = 0;
		virtual int clear(size_t offset, size_t size) = 0;
		virtual void flush() = 0;
		virtual std::string getName() const = 0;
	};

	template <size_t BS>
	struct StorageDevice: StorageDeviceBase {
		constexpr static size_t BlockSize = BS;

		StorageCache<(1 << 20 >> 3), BlockSize> cache;

		StorageDevice() = default;

		using StorageDeviceBase::flush;
		virtual void flush(StorageCacheEntry<BlockSize> &, uint64_t block) = 0;
	};

	struct StorageController {
		virtual ~StorageController() = default;
	};
}
