#ifndef KERNEL_H_
#define KERNEL_H_

#include <stddef.h>

#include "kernel_core.h"
#include "memory/Memory.h"

namespace DsOS {
	class Kernel {
		private:
			size_t memoryLow = 0;
			size_t memoryHigh = 0;
			Memory memory;

			void detectMemory();

		public:
			void main();
	};
}

#endif
