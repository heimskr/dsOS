#pragma once

#include <string>

#include "Defs.h"

namespace DsOS::Device {
	struct DeviceBase {
		virtual int read(void *buffer, size_t size, off_t offset) = 0;
		virtual int write(const void *buffer, size_t size, off_t offset) = 0;
		virtual int clear(off_t offset, size_t size);
		virtual std::string getName() const = 0;
	};
}
