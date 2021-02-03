#pragma once

#include "device/Device.h"
#include "hardware/AHCI.h"

namespace Thorn::Device {
	struct AHCIDevice: public DeviceBase {
		AHCI::Port *port;
		AHCIDevice() = delete;
		AHCIDevice(AHCI::Port *port_): port(port_) {}

		virtual int read(void *buffer, size_t size, size_t offset) override;
		virtual int write(const void *buffer, size_t size, size_t offset) override;
		virtual int clear(size_t offset, size_t size) override;
		virtual std::string getName() const override;
	};
}
