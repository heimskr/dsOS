#pragma once

#include "device/Storage.h"
#include "hardware/AHCI.h"

namespace Thorn {
	class AHCIDevice: public StorageDevice<AHCI::Port::BLOCKSIZE> {
		public:
			AHCI::Port *port;

			AHCIDevice(AHCI::Port *port_):
				port(port_) {}

			int read(void *buffer, size_t size, size_t offset) final;
			int write(const void *buffer, size_t size, size_t offset) final;
			int clear(size_t offset, size_t size) final;
			void flush() final;
			void flush(StorageCacheEntry<BlockSize> &, uint64_t block) final;
			std::string getName() const final;

		private:
			int readCache(void *buffer, size_t size, size_t offset);
			int writeCache(const void *buffer, size_t size, size_t offset);
	};
}
