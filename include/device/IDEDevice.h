#pragma once

#include "device/Device.h"
#include "hardware/IDE.h"

namespace DsOS::Device {
	struct IDEDevice: public DeviceBase {
		virtual int read(char *buffer, size_t size, off_t offset = 0) override;
		virtual int write(const void *buffer, size_t size, off_t offset = 0) override;
		virtual std::string getName() const override;
	};
}
