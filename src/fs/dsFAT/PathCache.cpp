#include "fs/dsFAT/dsFAT.h"
#include "fs/dsFAT/FDCache.h"
#include "fs/dsFAT/PathCache.h"

namespace DsOS::FS::DsFAT {
	PathCacheEntry::~PathCacheEntry() {
		if (complement)
			complement->complement = nullptr;
	}
}
