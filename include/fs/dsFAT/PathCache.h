#pragma once

#include <unordered_map>

#include "fs/dsFAT/Types.h"

namespace DsOS::FS::DsFAT {
	class DsFATDriver;
	struct FDCacheEntry;
	class PathCache;

	constexpr size_t PATHC_PATH_MAX = DSFAT_PATH_MAX + 1;

	struct PathCacheEntry {
		PathCache *parent;
		char path[PATHC_PATH_MAX];
		DirEntry entry;
		FDCacheEntry *complement;
		off_t offset;
		fd_t descriptor;

		~PathCacheEntry();
	};

	enum class PCInsertStatus {
		Success        =  0,
		Overwritten    =  1,
		DownFailed     = -1,
		FirstUpFailed  = -2,
		GaveUp         = -3,
		AliveFailed    = -4,
		SecondUpFailed = -5,
	};

	class PathCache {
		private:
			int overflowIndex = 0;

		public:
			DsFATDriver *parent;
			std::unordered_map<const char *, PathCacheEntry> pathMap;
			std::unordered_map<off_t, PathCacheEntry *> offsetMap;

			PathCache(DsFATDriver *parent_): parent(parent_) {}

			PathCacheEntry * find(const char *);
			PathCacheEntry * find(off_t);
			PathCacheEntry create(const char *, const DirEntry &, off_t);
			PCInsertStatus insert(const char *, const DirEntry &, off_t, PathCacheEntry **);
			bool erase(PathCacheEntry &);
			bool erase(const char *);
			bool erase(off_t);
	};
}
