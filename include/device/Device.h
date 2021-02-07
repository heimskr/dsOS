#pragma once

#include <string>

#include "Defs.h"

namespace Thorn::Device {
	struct DeviceBase {
		virtual int read(void *buffer, size_t size, size_t offset) = 0;
		virtual int write(const void *buffer, size_t size, size_t offset) = 0;
		virtual int clear(size_t offset, size_t size) = 0;
		virtual std::string getName() const = 0;
		virtual ~DeviceBase() {}
	};
}
