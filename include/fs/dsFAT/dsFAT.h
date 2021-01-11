#pragma once

// dsFAT: a primitive filesystem that shares some concepts with the FAT family of filesystems.

#include "fs/FS.h"

namespace DsOS::FS::DsFAT {
	class DsFATDriver: public Driver {
		public:
			virtual int rename(const char *path, const char *newpath) override;
			virtual int release(const char *path, FileInfo &) override;
			virtual int statfs(const char *, DriverStats &) override;
			virtual int utimens(const char *path, const timespec &) override;
			virtual int create(const char *path, mode_t mode, FileInfo &) override;
			virtual int write(const char *path, const char *buffer, size_t size, off_t offset, FileInfo &) override;
			virtual int mkdir(const char *path, mode_t mode) override;
			virtual int truncate(const char *path, off_t size) override;
			virtual int ftruncate(const char *path, off_t size, FileInfo &) override;
			virtual int rmdir(const char *path) override;
			virtual int unlink(const char *path) override;
			virtual int open(const char *path, FileInfo &) override;
			virtual int read(const char *path, char *buffer, size_t size, off_t offset, FileInfo &) override;
			virtual int readdir(const char *path, void *buffer, DirFiller filler, off_t offset, FileInfo &) override;
			virtual int getattr(const char *path, FileStats &) override;

			using Driver::Driver;
	};
}
