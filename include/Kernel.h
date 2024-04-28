#pragma once

#include "kernel_core.h"
#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "hardware/Keyboard.h"
#include "memory/Memory.h"

#include "Locked.h"
#include "Process.h"

#include <cstddef>

namespace Thorn {
	using PageFrameNumber = uint64_t;

	struct StorageController;
	struct StorageDeviceBase;

	class Kernel {
		private:
			Memory memory;

			Locked<x86_64::PageMeta4K> lockedPager;

			std::unique_ptr<Locked<ProcessMap, RecursiveMutex>> processes;
			std::unique_ptr<Locked<std::map<PageFrameNumber, PID>>> pageFrameProcesses;
			std::unique_ptr<Locked<std::vector<std::unique_ptr<StorageController>>>> storageControllers;
			std::unique_ptr<Locked<std::vector<std::unique_ptr<StorageDeviceBase>>>> storageDevices;

			PID lastPID = 1;

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

			void initPhysicalMemoryMap();

			/** Sets all page descriptors to zero. */
			void initPageDescriptors();

			PID nextPID();

		public:
			x86_64::PageTableWrapper kernelPML4;

			constexpr static PID MaxPID = std::numeric_limits<PID>::max() / 2;

			/** A region near the top of virtual memory is mapped to all physical memory. This address stores the start of that region. */
			uintptr_t physicalMemoryMap = 0;

			static Kernel *instance;
			static Kernel & getInstance();

			Kernel() = delete;
			Kernel(const x86_64::PageTableWrapper &pml4_);

			void main();

			Process & makeProcess();

			static void wait(size_t num_ticks, uint32_t frequency = 1);
			static void perish();

			void schedule();

			void onKey(Keyboard::InputKey, bool down);

			void initPointers();

			static void backtrace();
			static void backtrace(uintptr_t *);

			static x86_64::PageMeta4K & getPager(Lock<Mutex> &);

			static auto & getPageFrameProcesses(Lock<Mutex> &lock) {
				verifyInstance();
				return instance->pageFrameProcesses->get(lock);
			}

			static void verifyInstance() {
				if (!instance) {
					printf("Kernel instance is null!\n");
					perish();
				}
			}

			friend void runTests();
	};
}
