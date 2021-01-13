#pragma once

// dsFAT: a primitive filesystem that shares some concepts with the FAT family of filesystems.

#include <string>
#include <vector>

#include "fs/FS.h"
#include "fs/dsFAT/FDCache.h"
#include "fs/dsFAT/PathCache.h"
#include "fs/dsFAT/Types.h"

namespace DsOS::FS::DsFAT {
	class DsFATDriver: public Driver {
		private:
			bool ready = false;
			Superblock superblock;
			ssize_t blocksFree = -1;
			DirEntry root;
			void readSuperblock(Superblock &);
			void error(const std::string &);

			/** Attempts to find a file within the filesystem.
			 *  Note: can allocate new memory in *out or in **outptr.
			 *    fd          An optional file descriptor. If negative, it won't be used to find a file.
			 *   *path        An optional string representing a pathname. If NULL, it won't be used to find a file.
			 *   *out         A pointer that will be set to a copy of a matching file or directory.
			 *                Unused if outptr isn't NULL.
			 *  **outptr      A pointer that will be set to a pointer to a matching file or directory. Important for
			 *                keeping caches up to date.
			 *   *offset      A pointer that will be set to the directory entry's offset if a match was found.
			 *    get_parent  Whether to return the directory containing the match instead of the match itself.
			 *   *last_name   A pointer to a string that will be set to the last path component if get_parent is true.
			 *  Returns 0 if the operation was successful and no memory as allocated, returns 1 if the operation was
			 *  successful and allocated memory in *out or **outptr, returns a negative error code otherwise. */
			int find(fd_t, const char *, DirEntry *out, off_t *, bool get_parent, std::string *last_name);

			/** Gets the root directory of a disk image. Takes an optional pointer that will be filled with the root
			 *  directory's offset. Returns a pointer to a struct representing the partition's root directory. */
			DirEntry & getRoot(off_t * = nullptr);

			/** Reads all the directory entries in a given directory and stores them in an array.
			 *  Note: can allocate new memory in *entries and *offsets.
			 *   dir          A reference to a directory entry struct.
			 *   entries      A vector that will be filled with directory entries..
			 *  *offsets      A pointer that will be set to an array of raw offsets. Can be nullptr.
			 *  *first_index  An optional pointer that will be set to the index of the first entry other than ".." or
			 *                 ".".
			 *  Returns 0 if the operation succeeded or a negative error code otherwise. */
			int readDir(const DirEntry &dir, std::vector<DirEntry> &entries, std::vector<off_t> *offsets = nullptr,
			            int *first_index = nullptr);

			/** Reads the raw bytes for a given directory entry and stores them in an array.
			 *    file   A reference to a directory entry struct.
			 *    out    A reference that to a vector of bytes that will be filled with the read bytes.
			 *   *count  An optional pointer that will be set to the number of bytes read.
			 *  Returns 0 if the operation succeeded or a negative error code otherwise. */
			int readFile(const DirEntry &file, std::vector<uint8_t> &out, size_t *count = nullptr);

			block_t readFAT(size_t block_offset);
			void writeFAT(block_t block, size_t block_offset);

			bool isFree(const DirEntry &);

			bool checkBlock(block_t);

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

			DsFATDriver(Partition *);
			PathCache pathCache = this;
			FDCache fdCache = this;
	};
}
