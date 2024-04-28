#include "device/IDEDevice.h"
#include "memory/Memory.h"
#include "hardware/IDE.h"

namespace Thorn {
	int IDEDevice::read(void *buffer, size_t size, size_t offset) {
		if (offset % IDE::SECTOR_SIZE == 0 && size % IDE::SECTOR_SIZE == 0)
			return IDE::readSectors(ideID, size / IDE::SECTOR_SIZE, offset / IDE::SECTOR_SIZE, (char *) buffer);
		return IDE::readBytes(ideID, size, offset, buffer);
	}

	int IDEDevice::write(const void *buffer, size_t size, size_t offset) {
		if (offset % IDE::SECTOR_SIZE == 0 && size % IDE::SECTOR_SIZE == 0)
			return IDE::writeSectors(ideID, size / IDE::SECTOR_SIZE, offset / IDE::SECTOR_SIZE, buffer);
		return IDE::writeBytes(ideID, size, offset, buffer);
	}

	int IDEDevice::clear(size_t offset, size_t size) {
		if (offset % IDE::SECTOR_SIZE == 0 && size % IDE::SECTOR_SIZE == 0) {
			char buffer[IDE::SECTOR_SIZE] = {0};
			int status;
			for (size_t i = 0, max = size / IDE::SECTOR_SIZE; i < max; ++i) {
				status = IDE::writeSectors(ideID, 1, offset / IDE::SECTOR_SIZE + i, buffer);
				if (status != 0)
					return status;
			}
			return status;
		}

		size_t size_copy = size;
		size_t chunk_size = 1;
		while ((size_copy & 1) == 0 && chunk_size < IDE::SECTOR_SIZE) {
			size_copy /= 2;
			chunk_size *= 2;
		}

		char *buffer = new char[chunk_size];
		for (size_t i = 0, max = size / chunk_size; i < max; ++i) {
			int status = IDE::writeBytes(ideID, chunk_size, offset + chunk_size * i, buffer);
			if (status != 0)
				return status;
		}
		delete[] buffer;

		return 0;
	}

	std::string IDEDevice::getName() const {
		return IDE::devices[ideID].model;
	}
}
