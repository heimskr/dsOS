#include "device/AHCIDevice.h"
#include "memory/Memory.h"

namespace Thorn::Device {
	int AHCIDevice::read(void *buffer, size_t size, off_t offset) {
		if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0)
			return static_cast<int>(port->read(offset / AHCI::Port::BLOCKSIZE, size, buffer));
		return static_cast<int>(port->readBytes(size, offset, buffer));
	}

	int AHCIDevice::write(const void *buffer, size_t size, off_t offset) {
		if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0)
			return static_cast<int>(port->write(offset / AHCI::Port::BLOCKSIZE, size, buffer));
		return static_cast<int>(port->writeBytes(size, offset, buffer));
	}

	int AHCIDevice::clear(off_t offset, size_t size) {
		if (offset % AHCI::Port::BLOCKSIZE == 0 && size % AHCI::Port::BLOCKSIZE == 0) {
			char buffer[AHCI::Port::BLOCKSIZE] = {0};
			AHCI::Port::AccessStatus status = AHCI::Port::AccessStatus::Success;
			for (size_t i = 0, max = size / AHCI::Port::BLOCKSIZE; i < max; ++i) {
				status = port->write(offset / AHCI::Port::BLOCKSIZE + i, AHCI::Port::BLOCKSIZE, buffer);
				if (status != AHCI::Port::AccessStatus::Success)
					return static_cast<int>(status);
			}
			return static_cast<int>(status);
		}

		size_t size_copy = size;
		size_t chunk_size = 1;
		while ((size_copy & 1) == 0 && chunk_size < AHCI::Port::BLOCKSIZE) {
			size_copy /= 2;
			chunk_size *= 2;
		}

		char *buffer = new char[chunk_size];
		for (size_t i = 0, max = size / chunk_size; i < max; ++i) {
			AHCI::Port::AccessStatus status = port->writeBytes(chunk_size, offset + chunk_size * i, buffer);
			if (status != AHCI::Port::AccessStatus::Success)
				return static_cast<int>(status);
		}
		delete[] buffer;

		return 0;
	}

	std::string AHCIDevice::getName() const {
		char model[sizeof(ATA::DeviceInfo::model) + 1];
		port->getInfo().copyModel(model);
		return model;
	}
}
