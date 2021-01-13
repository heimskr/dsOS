#include "fs/dsFAT/dsFAT.h"
#include "fs/dsFAT/FDCache.h"
#include "fs/dsFAT/PathCache.h"

namespace DsOS::FS::DsFAT {
	PathCacheEntry::~PathCacheEntry() {
		if (complement)
			complement->complement = nullptr;
	}

	PathCacheEntry * PathCache::find(const char *path) {
		auto iter = map.find(path);
		return iter == map.end()? nullptr : &iter->second;
	}
}
