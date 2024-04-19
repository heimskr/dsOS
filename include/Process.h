#pragma once

#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"

#include <cstdint>
#include <map>

namespace Thorn {
	using PID = uint32_t;

	struct Process {
		PID pid;

		x86_64::PageMeta4K pager;
		x86_64::PageTableWrapper pageTable;
	};

	using ProcessMap = std::map<PID, Process>;
}
