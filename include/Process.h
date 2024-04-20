#pragma once

#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"

#include <cstdint>
#include <map>
#include <optional>

namespace Thorn {
	using PID = uint32_t;

	struct Process {
		PID pid;

		std::optional<x86_64::PageTableWrapper> pageTable;

		void init();

		void allocatePage(uintptr_t virtual_address);
	};

	using ProcessMap = std::map<PID, Process>;
}
