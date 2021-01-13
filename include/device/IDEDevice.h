#pragma once

#include "device/Device.h"

namespace DsOS::Device {
	struct IDEDevice: public DeviceBase {
		uint8_t ideID;
		IDEDevice() = delete;
		IDEDevice(uint8_t ide_id): ideID(ide_id) {}

		virtual int read(void *buffer, size_t size, off_t offset) override;
		virtual int write(const void *buffer, size_t size, off_t offset) override;
		virtual int clear(off_t offset, size_t size) override;
		virtual std::string getName() const override;
	};
}
