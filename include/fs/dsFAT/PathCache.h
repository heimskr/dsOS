#pragma once

#include "fs/dsFAT/Types.h"

namespace DsOS::FS::DsFAT {
	class DsFATDriver;
	struct FDCacheEntry;

	struct PathCacheEntry {
		bool alive = false;
		DirEntry entry;
		FDCacheEntry *complement;
		off_t offset;
		fd_t descriptor;

		~PathCacheEntry();
	};

	struct PathCache {
		DsFATDriver *parent;
		PathCacheEntry entries[PATHC_MAX];
		PathCache(DsFATDriver *parent_): parent(parent_) {}
		// Superblock superblock;
		// ssize_t blocksFree;
		// DirEntry root;
	};
}
