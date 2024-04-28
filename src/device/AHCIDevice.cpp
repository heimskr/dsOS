#include "device/AHCIDevice.h"
#include "memory/Memory.h"
#include "Kernel.h"

namespace Thorn {
	int AHCIDevice::read(void *buffer, size_t size, size_t offset) {
		// if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0)
		// 	return static_cast<int>(port->read(offset / AHCI::Port::BLOCKSIZE, size, buffer));
		// return static_cast<int>(port->readBytes(size, offset, buffer));
		return readCache(buffer, size, offset);
	}

	int AHCIDevice::write(const void *buffer, size_t size, size_t offset) {
		// if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0)
		// 	return static_cast<int>(port->write(offset / AHCI::Port::BLOCKSIZE, size, buffer));
		// return static_cast<int>(port->writeBytes(size, offset, buffer));
		return writeCache(buffer, size, offset);
	}

	int AHCIDevice::clear(size_t offset, size_t size) {
		int status{};
		char zeroes[BlockSize];

		while (size > BlockSize) {
			if (0 != (status = write(zeroes, BlockSize, offset)))
				return status;
			offset += BlockSize;
			size -= BlockSize;
		}

		return write(zeroes, size, offset);
	}

	std::string AHCIDevice::getName() const {
		char model[sizeof(ATA::DeviceInfo::model) + 1];
		port->getInfo().copyModel(model);
		return model;
	}

	void AHCIDevice::flush() {
		for (auto &[block, pair]: cache) {
			flush(pair.second, block);
		}
	}

	void AHCIDevice::flush(StorageCacheEntry<BlockSize> &entry, uint64_t block) {
		if (entry.flushed)
			return;

		const auto status = static_cast<int>(port->write(block, BlockSize, entry.data));

		if (!status) {
			printf("Failed to flush AHCI block %ld\n", block);
			Kernel::perish();
		}

		entry.flushed = true;
	}

	int AHCIDevice::readCache(void *buffer, size_t size, size_t offset) {
		int status{};

		auto advance = [&](size_t amount) {
			offset += amount;
			buffer = reinterpret_cast<char *>(buffer) + amount;
			size -= amount;
			return size == 0;
		};

		if (size_t rem = offset % BlockSize; rem != 0) {
			auto [ref, created] = cache[offset / BlockSize];
			StorageCacheEntry<BlockSize> &entry = ref.get();

			if (created) {
				if (0 != (status = read(entry.data, BlockSize, offset - rem)))
					return status;
				entry.flushed = true;
			}

			size_t affected = std::min(size, BlockSize - rem);
			std::memmove(buffer, &entry.data[rem], affected);
			if (advance(affected))
				return 0;
		}

		while (size >= BlockSize) {
			auto [ref, created] = cache[offset / BlockSize];
			StorageCacheEntry<BlockSize> &entry = ref.get();
			entry.flushed = false;
			std::memmove(buffer, entry.data, BlockSize);
			if (advance(BlockSize))
				return 0;
		}

		assert(size > 0);

		auto [ref, created] = cache[offset / BlockSize];
		StorageCacheEntry<BlockSize> &entry = ref.get();

		if (created) {
			if (0 != (status = read(entry.data, size, offset)))
				return status;
			entry.flushed = true;
		}

		std::memmove(buffer, entry.data, size);
		return 0;
	}

	int AHCIDevice::writeCache(const void *buffer, size_t size, size_t offset) {
		int status{};

		auto advance = [&](size_t amount) {
			offset += amount;
			buffer = reinterpret_cast<const char *>(buffer) + amount;
			size -= amount;
			return size == 0;
		};

		if (size_t rem = offset % BlockSize; rem != 0) {
			auto [ref, created] = cache[offset / BlockSize];
			StorageCacheEntry<BlockSize> &entry = ref.get();

			if (created) {
				if (0 != (status = read(entry.data, BlockSize, offset - rem)))
					return status;
			} else {
				entry.flushed = false;
			}

			size_t affected = std::min(size, BlockSize - rem);
			std::memmove(&entry.data[rem], buffer, affected);
			if (advance(affected))
				return 0;
		}

		while (size >= BlockSize) {
			auto [ref, created] = cache[offset / BlockSize];
			StorageCacheEntry<BlockSize> &entry = ref.get();
			entry.flushed = false;
			std::memmove(entry.data, buffer, BlockSize);
			if (advance(BlockSize))
				return 0;
		}

		assert(size > 0);

		auto [ref, created] = cache[offset / BlockSize];
		StorageCacheEntry<BlockSize> &entry = ref.get();

		if (created) {
			if (0 != (status = read(entry.data, size, offset)))
				return status;
		} else {
			entry.flushed = false;
		}

		std::memmove(entry.data, buffer, size);
		return 0;
	}
}
