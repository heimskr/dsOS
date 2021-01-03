#ifndef KERNEL_H_
#define KERNEL_H_

#include <stddef.h>

#include "kernel_core.h"
#include "memory/Memory.h"
#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"

namespace DsOS {
	class Kernel {
		private:
			size_t memoryLow = 0;
			size_t memoryHigh = 0;
			Memory memory;

			/** The area where page descriptors are stored. */
			char *pageDescriptors = nullptr;

			/** The length of the pageDescriptors area in bytes. */
			size_t pageDescriptorsLength = 0;

			/** The area where actual pages are stored. */
			void *pagesStart = nullptr;

			/** The size of the area where actual pages are stored in bytes. */
			size_t pagesLength = 0;

			/** Uses data left behind by multiboot to determine the boundaries of physical memory. */
			void detectMemory();

			/** Carves the usable region of physical memory into a section for page descriptors and a section for
			 *  pages. */
			void arrangeMemory();

			/** Sets all page descriptors to zero. */
			void initPageDescriptors();

		public:
			x86_64::PageTableWrapper kernelPML4;
			x86_64::PageMeta4K pageMeta;

			static Kernel *instance;

			Kernel() = delete;
			Kernel(const x86_64::PageTableWrapper &pml4_);

			void main();

			static void wait(size_t millimoments = 1000);
	};
}

#endif
