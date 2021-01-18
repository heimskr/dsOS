#include "hardware/AHCI.h"

namespace DsOS::AHCI {
	PCI::Device *controller = nullptr;
	HBAMemory *abar = nullptr;

	const char *deviceTypes[5] = {"Null", "SATA", "SEMB", "PortMultiplier", "SATAPI"};

	DeviceType identifyDevice(volatile HBAPort &port) {
		const uint32_t ssts = port.ssts;
		const uint8_t ipm = (ssts >> 8) & 0x0f;
		const uint8_t det = ssts & 0x0f;

		if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
			return DeviceType::Null;

		switch (port.sig) {
			case SIG_ATAPI:
				return DeviceType::SATAPI;
			case SIG_SEMB:
				return DeviceType::SEMB;
			case SIG_PM:
				return DeviceType::PortMultiplier;
			default:
				return DeviceType::SATA;
		}
	}

	int getCommandSlot(volatile HBAPort &port) {
		int slots = port.sact | port.ci;
		const int command_slots = (abar->cap & 0xf00) >> 8;
		for (int i = 0; i < command_slots; ++i) {
			if ((slots & 1) == 0)
				return i;
			slots >>= 1;
		}

		return -1;
	}
}
