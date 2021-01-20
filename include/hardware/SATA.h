#pragma once

#include "hardware/AHCI.h"
#include "hardware/ATA.h"

namespace DsOS::SATA {
	bool issueCommand(volatile AHCI::HBAPort &, ATA::Command, bool write, void *buffer, uint16_t prdtl,
	                  uint32_t byte_count, uint64_t start, uint16_t count);
	bool read(volatile AHCI::HBAPort &, uint64_t start, uint32_t count, void *buffer);
}
