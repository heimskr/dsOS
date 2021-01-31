#pragma once

#include "hardware/AHCI.h"
#include "hardware/ATA.h"

namespace DsOS::SATA {
	bool issueCommand(AHCI::Controller &, AHCI::Port &, ATA::Command, bool write, void *buffer, uint16_t prdtl,
	                  uint32_t byte_count, uint64_t start);
	bool read(AHCI::Controller &, AHCI::Port &, uint64_t start, uint32_t count, void *buffer);
}
