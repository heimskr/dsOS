#pragma once

#include "fs/ThornFAT/Types.h"

namespace Thorn::FS::ThornFAT {
	class ThornFATDriver;
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
		ThornFATDriver *parent;
		FDCacheEntry entries[FDC_MAX];
		
		FDCache(ThornFATDriver *parent_): parent(parent_) {}

		FDCacheEntry * find(fd_t);
	};
}
