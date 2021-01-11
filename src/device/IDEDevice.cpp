#include "device/IDEDevice.h"
#include "hardware/IDE.h"

namespace DsOS::Device {
	int IDEDevice::read(void *buffer, size_t size, off_t offset) {
		if (offset % IDE::SECTOR_SIZE == 0 && size % IDE::SECTOR_SIZE == 0)
			return IDE::readSectors(ideID, size / IDE::SECTOR_SIZE, offset / IDE::SECTOR_SIZE, (char *) buffer);
		return IDE::readBytes(ideID, size, offset, buffer);
	}

	int IDEDevice::write(const void *buffer, size_t size, off_t offset) {
		if (offset % IDE::SECTOR_SIZE == 0 && size % IDE::SECTOR_SIZE == 0)
			return IDE::writeSectors(ideID, size / IDE::SECTOR_SIZE, offset / IDE::SECTOR_SIZE, buffer);
		return IDE::writeBytes(ideID, size, offset, buffer);
	}

	std::string IDEDevice::getName() const {
		return IDE::devices[ideID].model;
	}
}
