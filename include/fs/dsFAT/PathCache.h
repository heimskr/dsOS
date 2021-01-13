#pragma once

#include <unordered_map>

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
		std::unordered_map<const char *, PathCacheEntry> map;

		PathCache(DsFATDriver *parent_): parent(parent_) {}

		PathCacheEntry * find(const char *);
	};
}
