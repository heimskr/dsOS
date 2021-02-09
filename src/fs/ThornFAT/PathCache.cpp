#include "fs/ThornFAT/ThornFAT.h"
#include "fs/ThornFAT/FDCache.h"
#include "fs/ThornFAT/PathCache.h"
#include "fs/ThornFAT/Util.h"
#include "lib/printf.h"

namespace Thorn::FS::ThornFAT {
	PathCacheEntry::~PathCacheEntry() {
		if (complement)
			complement->complement = nullptr;
	}

	PathCacheEntry * PathCache::find(const char *path) {
		return nullptr; // PathCache is deprecated.
		// auto iter = pathMap.find(path);
		// return iter == pathMap.end()? nullptr : &iter->second;
	}

	PathCacheEntry * PathCache::find(off_t offset) {
		return nullptr;
		// auto iter = offsetMap.find(offset);
		// return iter == offsetMap.end()? nullptr : iter->second;
	}

	PathCacheEntry PathCache::create(const char *path, const DirEntry &entry, off_t offset) {
		printf("\e[36m[PathCache::\e[1mcreate\e[22m(\"%s\", %s, %ld)]\e[0m\n", path, std::string(entry).c_str(), offset);
		PathCacheEntry out = {
			.parent     = this,
			.path       = {0},
			.entry      = entry,
			.complement = nullptr,     // Optional; it's up to the caller to set this.
			.offset     = offset,
			.descriptor = FDC_MAX + 1  // This is likewise optional.
		};

		// PathCache::insert, the only public way of changing the path cache, overwrites most fields of a PathCacheEntry
		// in the path cache but not its mutex (TODO: implement mutexes). Elsewhere too, when an entry is ostensibly
		// outright replaced, its mutex will be preserved and assigned to the replacement. The mutexes thus protect both
		// the entries themselves as well as their indices in the entries array.
		// int status = pthread_mutex_init(out.lock, NULL);
		// if (status) {
			// DIE(PCNEWH, "Mutex initialization failed for " BSR, path);
		// }

		strncpy(out.path, path, PATHC_PATH_MAX);
		return out;
	}

	PCInsertStatus PathCache::insert(const char *path, const DirEntry &entry, off_t offset, PathCacheEntry **out) {
		serprintf("\e[36m[PathCache::\e[1minsert\e[22m(\"%s\", %s, %ld, 0x%lx)]\e[0m\n", path, std::string(entry).c_str(), offset, out);
		entry.print();
		// TODO: mutexes.

		// ENTER;
		// pathc_t *files = pcache.files;

		// If there's already an entry with the same path, copy the new information into it.
		auto found_iter = pathMap.find(path);
		if (found_iter != pathMap.end()) {
			PathCacheEntry &found = found_iter->second;
			found.entry = entry;
			found.offset = offset;
			if (found.complement) {
				found.complement->alive = false;
				// parent->fdCache.release(found.complement->descriptor);
				found.descriptor = 0;
			}

			if (out)
				*out = &found;

			return PCInsertStatus::Success;
		}

		// If there's no item with the same path already in the cache,
		// create a new entry and try to put it in an empty slot.
		PathCacheEntry created = create(path, entry, offset);

		if (pathMap.size() < PATHC_MAX) {
			auto pair = pathMap.insert({path, created});
			offsetMap.insert({offset, &pair.first->second});
			if (out)
				*out = &pair.first->second;
			return PCInsertStatus::Success;
		}

		// DBG(PCINSERTH, "trylock(alive)...");
		// int foo = pthread_mutex_trylock(&pcache.lock_alive);
		// DBGN(PCINSERTH, "trylock(alive) " UDARR, foo);

		// Otherwise, pick a slot to overwrite.
		// I'm doing it this way because if we chose the same slot each time, we'd keep overwriting the last-inserted
		// entry and the rest of the array could very well contain entries that won't ever be used again that'll just
		// stay there for all eternity. That wouldn't be good, so we round-robin the slot to replace.
		// TODO: implement mutexes.

		// int tries = 0, try_;
		// pathc_t *locked = NULL;
		// for (;;) {
		// 	try_ = pathc_trylock(&files[pathc_overflow_index]);

		// 	if (try_) {
		// 		overflowIndex = (overflowIndex + 1) % PATHC_MAX;
		// 	} else {
		// 		locked = &files[pathc_overflow_index];
		// 		break;
		// 	}

		// 	if (++tries == PATHC_MAX) {
		// 		printf("[PathCache::insert] Giving up.\n");
		// 		return PCInsertStatus::GaveUp;
		// 	}
		// }

		const int map_offset = overflowIndex % pathMap.size();

		auto overwrite_iter = std::next(pathMap.begin(), map_offset);
		overwrite_iter->second = std::move(created);
		if (out)
			*out = &overwrite_iter->second;

		DBGF(PCINSERTH, "Overwrote a path cache entry at index %lu.", map_offset);
		overflowIndex = (overflowIndex + 1) % PATHC_MAX;

		// if (locked != NULL) {
		// 	PC_UP_L(*locked, PCI_SECOND_UP_FAILED);
		// }

		// EXIT;
		return PCInsertStatus::Overwritten;
	}

	bool PathCache::erase(PathCacheEntry &entry) {
		offsetMap.erase(entry.offset);
		for (auto iter = pathMap.begin(); iter != pathMap.end(); ++iter) {
			if (&entry == &iter->second) {
				pathMap.erase(iter);
				return true;
			}
		}
		return false;
	}

	bool PathCache::erase(const char *path) {
		auto iter = pathMap.find(path);
		if (iter == pathMap.end())
			return false;
		offsetMap.erase(iter->second.offset);
		pathMap.erase(iter);
		return true;
	}

	bool PathCache::erase(off_t offset) {
		if (PathCacheEntry *entry = find(offset)) {
			erase(*entry);
			return true;
		}
		return false;
	}

	void PathCache::clear() {
		pathMap.clear();
		offsetMap.clear();
	}
}
