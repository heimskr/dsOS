#ifndef KERNEL_H_
#define KERNEL_H_

#include <stddef.h>

#include "kernel_core.h"

namespace DsOS {
	class Kernel {
		private:
			size_t memoryLow = 0;
			size_t memoryHigh = 0;

			void detectMemory();

		public:
			void main();
	};
}

#endif
