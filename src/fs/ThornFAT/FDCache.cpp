#include "fs/ThornFAT/ThornFAT.h"
#include "fs/ThornFAT/FDCache.h"
#include "fs/ThornFAT/PathCache.h"

namespace Thorn::FS::ThornFAT {
	FDCacheEntry::~FDCacheEntry() {
		if (complement)
			complement->complement = nullptr;
	}

	FDCacheEntry * FDCache::find(fd_t fd) {
		return fd != UINT64_MAX && fd < FDC_MAX? &entries[fd] : nullptr;
	}
}
