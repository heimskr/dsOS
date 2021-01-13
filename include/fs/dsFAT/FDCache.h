#pragma once

#include "fs/dsFAT/Types.h"

namespace DsOS::FS::DsFAT {
	class DsFATDriver;
	struct PathCacheEntry;

	struct FDCache;

	struct FDCacheEntry {
		FDCache *parent;
		bool alive;
		fd_t descriptor;
		PathCacheEntry *complement;

		~FDCacheEntry();
	};

	struct FDCache {
		DsFATDriver *parent;
		FDCacheEntry entries[FDC_MAX];
		
		FDCache(DsFATDriver *parent_): parent(parent_) {}

		FDCacheEntry * find(fd_t);
	};
}
