#include "fs/dsFAT/dsFAT.h"
#include "fs/dsFAT/FDCache.h"
#include "fs/dsFAT/PathCache.h"

namespace DsOS::FS::DsFAT {
	FDCacheEntry::~FDCacheEntry() {
		if (complement)
			complement->complement = nullptr;
	}

	FDCacheEntry * FDCache::find(fd_t fd) {
		return fd != UINT64_MAX && fd < FDC_MAX? &entries[fd] : nullptr;
	}
}
