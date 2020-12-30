#pragma once

#include <stdint.h>

#include "mmu.h"

namespace x86_64 {
	struct PageTable {
		enum class Type {PML4, PDP, PD, PT};

		void **entries;
		Type type;

		PageTable(void **, Type);
		void clear();
		void print();
	};
}
