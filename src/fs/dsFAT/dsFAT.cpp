#include <cerrno>

#include "fs/dsFAT/dsFAT.h"
#include "fs/dsFAT/Types.h"
#include "lib/printf.h"
#include "memory/Memory.h"

#define FD_VALID(fd) ((fd) != UINT64_MAX)

namespace DsOS::FS::DsFAT {
	DsFATDriver::DsFATDriver(Partition *partition_): Driver(partition_) {
		root.startBlock = -1;
		readSuperblock(superblock);
	}

	void DsFATDriver::readSuperblock(Superblock &out) {
		partition->read(&out, sizeof(Superblock), 0);
	}

	void DsFATDriver::error(const std::string &err) {
		printf("Error: %s\n", err.c_str());
		for (;;);
	}

	int DsFATDriver::find(fd_t fd, const char *path, DirEntry *out, DirEntry **outptr, off_t *offset, bool get_parent,
	                      std::string *last_name) {
		if (!FD_VALID(fd) && !path)
			error("Both search types are unspecified.");

		int status = 0;

		if (!get_parent) {
			// If we're not trying to find the parent directory, then we
			// can try to find the information in either of the caches.
			PathCacheEntry *pcache = nullptr;

			if (FD_VALID(fd)) {
				// Try to find the file in the cache based on the file descriptor if one's provided.
				FDCacheEntry *fc_entry = fdCache.find(fd);
				if (fc_entry) {
					pcache = fc_entry->complement;
				} else if (!path) {
					// If a file descriptor was the only criterion, we don't have enough to continue.
					return -ENOENT;
				}
			}
		}
	}

	int DsFATDriver::rename(const char *path, const char *newpath) {
		return 0;
	}

	int DsFATDriver::release(const char *path, FileInfo &) {
		return 0;
	}

	int DsFATDriver::statfs(const char *, DriverStats &) {
		return 0;
	}

	int DsFATDriver::utimens(const char *path, const timespec &) {
		return 0;
	}

	int DsFATDriver::create(const char *path, mode_t mode, FileInfo &) {
		return 0;
	}

	int DsFATDriver::write(const char *path, const char *buffer, size_t size, off_t offset, FileInfo &) {
		return 0;
	}

	int DsFATDriver::mkdir(const char *path, mode_t mode) {
		return 0;
	}

	int DsFATDriver::truncate(const char *path, off_t size) {
		return 0;
	}

	int DsFATDriver::ftruncate(const char *path, off_t size, FileInfo &) {
		return 0;
	}

	int DsFATDriver::rmdir(const char *path) {
		return 0;
	}

	int DsFATDriver::unlink(const char *path) {
		return 0;
	}

	int DsFATDriver::open(const char *path, FileInfo &) {
		return 0;
	}

	int DsFATDriver::read(const char *path, char *buffer, size_t size, off_t offset, FileInfo &) {
		return 0;
	}

	int DsFATDriver::readdir(const char *path, void *buffer, DirFiller filler, off_t offset, FileInfo &) {
		return 0;
	}

	int DsFATDriver::getattr(const char *path, FileStats &) {
		return 0;
	}
}
