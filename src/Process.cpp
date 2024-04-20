#include "Kernel.h"
#include "Process.h"

namespace Thorn {
	void Process::init() {
		Lock<Mutex> pager_lock;
		x86_64::PageMeta4K &pager = Kernel::getPager(pager_lock);
		uint64_t *pml4 = reinterpret_cast<uint64_t *>(pager.allocateFreePhysicalAddress());
		memset(pml4, 0, THORN_PAGE_SIZE);
		pageTable.emplace(pml4, x86_64::PageTableWrapper::Type::PML4);
	}

	void Process::allocatePage(uintptr_t virtual_address) {
		Lock<Mutex> pager_lock, frames_lock;
		x86_64::PageMeta4K &pager = Kernel::getPager(pager_lock);
		auto &page_frames = Kernel::getPageFrameProcesses(frames_lock);
		PageFrameNumber pfn = pager.allocateFreePhysicalFrame();
		page_frames[pfn] = pid;
	}
}
