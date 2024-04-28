#include "device/AHCIDevice.h"
#include "memory/Memory.h"

namespace Thorn {
	int AHCIDevice::read(void *buffer, size_t size, size_t offset) {
		if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0)
			return static_cast<int>(port->read(offset / AHCI::Port::BLOCKSIZE, size, buffer));
		return static_cast<int>(port->readBytes(size, offset, buffer));
	}

	int AHCIDevice::write(const void *buffer, size_t size, size_t offset) {
		if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0)
			return static_cast<int>(port->write(offset / AHCI::Port::BLOCKSIZE, size, buffer));
		return static_cast<int>(port->writeBytes(size, offset, buffer));
	}

	int AHCIDevice::clear(size_t offset, size_t size) {
		if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0) {
			char buffer[AHCI::Port::BLOCKSIZE] = {0};
			AHCI::Port::AccessStatus status = AHCI::Port::AccessStatus::Success;
			size_t last_pct = 0;
			printf("0%%\n");
			for (size_t i = 0, max = size / AHCI::Port::BLOCKSIZE; i < max; ++i) {
				status = port->write(offset / AHCI::Port::BLOCKSIZE + i, AHCI::Port::BLOCKSIZE, buffer);
				if (status != AHCI::Port::AccessStatus::Success) {
					printf("[%d] Whoops: %d\n", __LINE__, status);
					return static_cast<int>(status);
				}

				if (size_t new_pct = i * 100 / max; new_pct != last_pct) {
					last_pct = new_pct;
					serprintf("\e[F");
					printf("%lu%%\n", new_pct);
				}
			}
			printf("[%d] Whoops: %d\n", __LINE__, status);
			return static_cast<int>(status);
		}

		size_t size_copy = size;
		size_t chunk_size = 1;
		while ((size_copy & 1) == 0 && chunk_size < AHCI::Port::BLOCKSIZE) {
			size_copy /= 2;
			chunk_size *= 2;
		}

		auto buffer = std::make_unique<char[]>(chunk_size);
		for (size_t i = 0, max = size / chunk_size; i < max; ++i) {
			AHCI::Port::AccessStatus status = port->writeBytes(chunk_size, offset + chunk_size * i, buffer.get());
			if (status != AHCI::Port::AccessStatus::Success) {
				printf("[%d] Whoops: %d\n", __LINE__, status);
				return static_cast<int>(status);
			}
		}

		return 0;
	}

	std::string AHCIDevice::getName() const {
		char model[sizeof(ATA::DeviceInfo::model) + 1];
		port->getInfo().copyModel(model);
		return model;
	}
}
