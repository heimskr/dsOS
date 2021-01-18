#include "hardware/SATA.h"

namespace DsOS::SATA {
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
				return DeviceType::PM;
			default:
				return DeviceType::SATA;
		}
	}

	const char *deviceTypes[5] = {"Null", "SATA", "SEMB", "PM", "SATAPI"};
}
