#pragma once

#include "device/Storage.h"
#include "hardware/IDE.h"

namespace Thorn {
	struct IDEDevice: StorageDevice<IDE::SECTOR_SIZE> {
		uint8_t ideID;
		IDEDevice() = delete;
		IDEDevice(uint8_t ide_id): ideID(ide_id) {}

		int read(void *buffer, size_t size, size_t offset) final;
		int write(const void *buffer, size_t size, size_t offset) final;
		int clear(size_t offset, size_t size) final;
		void flush() final {}
		void flush(StorageCacheEntry<BlockSize> &, uint64_t) final {}
		std::string getName() const final;
	};
}
