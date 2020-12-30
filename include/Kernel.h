#ifndef KERNEL_H_
#define KERNEL_H_

#include <stddef.h>

#include "kernel_core.h"
#include "memory/Memory.h"
#include "arch/x86_64/PageTable.h"

namespace DsOS {
	class Kernel {
		private:
			size_t memoryLow = 0;
			size_t memoryHigh = 0;
			Memory memory;
			x86_64::PageTable *pml4;

			void detectMemory();

		public:
			static Kernel *instance;

			Kernel() = delete;
			Kernel(x86_64::PageTable *pml4_);

			void main();
	};
}

#endif
