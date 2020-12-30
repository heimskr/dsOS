#pragma once

#include <stdint.h>

#include "mmu.h"

namespace x86_64 {
	struct PageTable {
		uint64_t entries[PML4_SIZE];
	};
}
