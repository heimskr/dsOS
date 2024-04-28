#pragma once

#include "device/Storage.h"
#include "hardware/AHCI.h"

namespace Thorn {
	struct AHCIDevice: public StorageDevice {
		AHCI::Port *port;
		AHCIDevice() = delete;
		AHCIDevice(AHCI::Port *port_): port(port_) {}
		virtual ~AHCIDevice() {}

		virtual int read(void *buffer, size_t size, size_t offset) override;
		virtual int write(const void *buffer, size_t size, size_t offset) override;
		virtual int clear(size_t offset, size_t size) override;
		virtual std::string getName() const override;
	};
}
