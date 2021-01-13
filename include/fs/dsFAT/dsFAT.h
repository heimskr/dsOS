#pragma once

// dsFAT: a primitive filesystem that shares some concepts with the FAT family of filesystems.

#include <string>
#include <vector>

#include "fs/FS.h"
#include "fs/dsFAT/FDCache.h"
#include "fs/dsFAT/PathCache.h"
#include "fs/dsFAT/Types.h"

namespace DsOS::FS::DsFAT {
	constexpr uint32_t MAGIC = 0xfa91283e;
	constexpr int NEWFILE_SKIP_MAX = 4;
	constexpr int OVERFLOW_MAX = 32;
	constexpr size_t MINBLOCKS = 3;

	class DsFATDriver: public Driver {
		private:
			Superblock superblock;
			ssize_t blocksFree = -1;
			DirEntry root;

			int writeSuperblock(const Superblock &);
			int readSuperblock(Superblock &);
			void error(const std::string &);

			/** Attempts to find a file within the filesystem. 
			 *  Note: can allocate new memory in *out or in **outptr.
			 *  @param fd         An optional file descriptor. If negative, it won't be used to find a file.
			 *  @param path       An optional string representing a pathname. If nullptr, it won't be used to find a
			 *                    file.
			 *  @param out        A pointer that will be set to a copy of a matching file or directory. Unused if outptr
			 *                    isn't nullptr.
			 *  @param outptr     A pointer that will be set to a pointer to a matching file or directory. Important for
			 *                    keeping caches up to date.
			 *  @param offset     A pointer that will be set to the directory entry's offset if a match was found.
			 *  @param get_parent Whether to return the directory containing the match instead of the match itself.
			 *  @param last_name  A pointer to a string that will be set to the last path component if get_parent is
			 *                    true.
			 *  @return Returns 0 if the operation was successful and no memory as allocated, returns 1 if the operation
			 *          was successful and memory was allocated in *out or **outptr, returns a negative error code
			 *          otherwise. */
			int find(fd_t, const char *, DirEntry *out, DirEntry **outptr, off_t *, bool get_parent,
			         std::string *last_name);

			/** Removes a chain of blocks from the file allocation table. 
			 *  Returns the number of blocks that were freed. */
			size_t forget(block_t start);

			/** Writes a directory entry at a given offset. 
			 *  Returns 0 if the operation was successful or a negative error code otherwise. */
			int writeEntry(const DirEntry &, off_t);

			/** Gets the root directory of a disk image. Takes an optional pointer that will be filled with the root
			 *  directory's offset. Returns a pointer to a struct representing the partition's root directory. */
			DirEntry & getRoot(off_t * = nullptr);

			/** Reads all the directory entries in a given directory and stores them in an array. 
			 *  Note: can allocate new memory in *entries and *offsets.
			 *  @param dir         A reference to a directory entry struct.
			 *  @param entries     A vector that will be filled with directory entries.
			 *  @param offsets     A pointer that will be set to an array of raw offsets. Can be nullptr.
			 *  @param first_index An optional pointer that will be set to the index of the first entry other than ".."
			 *                     or ".".
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise. */
			int readDir(const DirEntry &dir, std::vector<DirEntry> &entries, std::vector<off_t> *offsets = nullptr,
			            int *first_index = nullptr);

			/** Reads the raw bytes for a given directory entry and stores them in an array.
			 *  @param file  A reference to a directory entry struct.
			 *  @param out   A reference that to a vector of bytes that will be filled with the read bytes.
			 *  @param count An optional pointer that will be set to the number of bytes read.
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise. */
			int readFile(const DirEntry &file, std::vector<uint8_t> &out, size_t *count = nullptr);

			/** Creates a new file.
			 *  @param path              The path of the new file to create.
			 *  @param length            The length (in bytes) of the file's content.
			 *  @param type              The type of the new entry (whether it's a file or a directory).
			 *  @param times             An optional pointer to a set of times that will be assigned to the entry. If
			 *                           nullptr, the current time (or a constant if CONSTANT_TIME is defined) will be
			 *                           used.
			 *  @param dir_out           An optional pointer to which a pointer to the new entry will be written.
			 *  @param offset_out        An optional pointer to which the offset of the new directory entry will be
			 *                           written.
			 *  @param parent_dir_out    An optional pointer to which a pointer to the new entry's parent directory
			 *                           entry will be written.
			 *  @param parent_offset_out An optional pointer to which the new entry's parent directory entry's offset
			 *                           will be written.
			 *  @param noalloc           A parameter indicating whether blocks shouldn't be allocated for the new file.
			 *                           This is useful if you just want to create a dummy entry (e.g., in fat_rename).
			 *  @return Returns 0 if the operation succeeded and **dir_out doesn't contain newly allocated memory,
			 *          returns 1 if the operation succeeded and **dir_out does contain newly allocated memory, returns
			 *          a negative number otherwise. */
			int newFile(const char *path, uint32_t length, FileType type, const Times *times, DirEntry **dir_out,
			            off_t *offset_out, DirEntry **parent_dir_out, off_t *parent_offset_out, bool noalloc);

			/** Attempts to remove a file.
			 *  @param path          A file path.
			 *  @param remove_pentry Whether to remove the entry (by path) from the path cache.
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise.
			 */
			int remove(const char *path, bool remove_pentry);

			/** Attempts to find a free block in a file allocation table.
			 *  @return Returns the index of the first free block if any were found; -1 otherwise. */
			block_t findFreeBlock();

			void initFAT(size_t table_size, size_t block_size);
			block_t readFAT(size_t block_offset);
			int writeFAT(block_t block, size_t block_offset);

			void initData(size_t block_count, size_t table_size);

			bool hasFree(const size_t);
			bool isFree(const DirEntry &);
			ssize_t countFree();

			bool checkBlock(block_t);

			size_t writeOffset = 0;

			template <typename T>
			int writeMany(T n, size_t count, off_t offset) {
				int status;
				for (size_t i = 0; i < count; ++i) {
					status = partition->write(&n, sizeof(T), offset + i * sizeof(T));
					if (status != 0) {
						printf("[DsFATDriver::writeInt] Writing failed: %s\n", strerror(status));
						return -status;
					}
				}
				return 0;
			}

			template <typename T>
			int writeMany(T n, size_t count) {
				int status;
				for (size_t i = 0; i < count; ++i) {
					status = partition->write(&n, sizeof(T), writeOffset);
					if (status != 0) {
						printf("[DsFATDriver::writeInt] Writing failed: %s\n", strerror(status));
						return -status;
					}
					writeOffset += sizeof(T);
				}
				return 0;
			}

			template <typename T>
			int write(const T &n) {
				int status = partition->write(&n, sizeof(T), writeOffset);
				if (status != 0) {
					printf("[DsFATDriver::write] Writing failed: %s\n", strerror(status));
					return -status;
				}
				writeOffset += sizeof(T);
				return 0;
			}

			static size_t tableSize(size_t block_count, size_t block_size);

			/** Ugly hack to avoid allocating memory on the heap because I'm too lazy to deal with freeing it. */
			DirEntry overflow[OVERFLOW_MAX];
			size_t overflowIndex = 0;

			static char nothing[sizeof(DirEntry)];

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
			bool make(uint32_t block_size);

			DsFATDriver(Partition *);
			PathCache pathCache = this;
			FDCache fdCache = this;

	};
}
