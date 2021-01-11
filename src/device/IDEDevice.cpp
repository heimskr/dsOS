#include "device/IDEDevice.h"

namespace DsOS::Device {
	int IDEDevice::read(char *buffer, size_t size, off_t offset) {
		return 0;
	}

	int IDEDevice::write(const void *buffer, size_t size, off_t offset) {
		return 0;
	}

	std::string IDEDevice::getName() const {
		return "None";
	}
}
