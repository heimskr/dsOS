#include <cerrno>
#include <string>

#include "fs/dsFAT/dsFAT.h"
#include "fs/dsFAT/Types.h"
#include "fs/dsFAT/Util.h"
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

	int DsFATDriver::find(fd_t fd, const char *path, DirEntry *out, off_t *offset, bool get_parent,
	                      std::string *last_name) {
		// ENTER;

		if (!FD_VALID(fd) && !path) {
			error("Both search types are unspecified.");
			// EXIT;
			return -EINVAL;
		}

		int status = 0;

// #ifndef DEBUG_FATFIND
// 	METHOD_OFF(INDEX_FATFIND);
// #endif

		if (!get_parent) {
			// If we're not trying to find the parent directory, then we
			// can try to find the information in either of the caches.
			PathCacheEntry *pc_entry = nullptr;

			// DBGFE(FATFINDH, CHMETHOD("fat_find") "fd " BFDR ", path " BSTR, fd, path == NULL? "null" : path);

			if (FD_VALID(fd)) {
				// Try to find the file in the cache based on the file descriptor if one's provided.
				FDCacheEntry *fc_entry = fdCache.find(fd);
				if (fc_entry) {
					// DBGN(FATFINDH, "Found from " ICS("fdcache") " for file descriptor", fd);
					pc_entry = fc_entry->complement;
				} else if (!path) {
					// If a file descriptor was the only criterion, we don't have enough to continue.
					// FF_EXIT;
					return -ENOENT;
				}
			}

			if (!pc_entry) {
				// If we couldn't find a file by file descriptor, then we'll have to try to find it by path instead.
				pc_entry = pathCache.find(path);
				// if (pc_entry != NULL)
				// 	DBGF(FATFINDH, "Found from " IMS("pcache") " for path " BSTR DM " offset " BLR, path, pc_entry->offset);
			}

			if (pc_entry) {
				// We found something. Store its entry and its offset and we're done.
				// const char *filename = pc_entry->entry.name.str;
				// if (strlen(fname) == 0)
				// 	WARNS(FATFINDH, SUB "pc_entry" DARR "entry" DARR "fname: " A_RED "empty" A_RESET);

				if (offset)
					*offset = pc_entry->offset;
				if (out)
					*out = pc_entry->entry;

				// DBG(FATFINDH, "Returning from cache."); FF_EXIT;
				return status;
			}
		}

		if (strcmp(path, "/") == 0 || strlen(path) == 0) {
			// For the special case of "/" or "" (maybe that last one should be invalid...), return the root directory.
			DirEntry &rootdir = getRoot(offset);
			if (out)
				*out = rootdir;

			// DBG(FATFINDH, "Returning the root."); FF_EXIT;
			return status;
		}

		size_t count;
		size_t i;
		bool at_end;

		std::vector<DirEntry> entries;
		std::vector<off_t> offsets;
		std::string newpath, remaining = path;

		// Start at the root.
		DirEntry &dir = getRoot();
		off_t dir_offset = superblock.startBlock * superblock.blockSize;

		bool done = false;

		do {
			std::optional<std::string> search = Util::pathFirst(remaining, &newpath);
			remaining = newpath;
			at_end = newpath.empty() || !search.has_value();

			if (at_end && get_parent) {
				// We just want the containing directory, thank you very much.
				if (out)
					*out = dir;

				if (offset)
					*offset = dir_offset;
				// WARNS(FATFINDH, "Returning path component.");
				if (last_name)
					*last_name = search.value_or("");
				// FREE(remaining);
				// FF_EXIT;
				return status;
			}

			if (!search.has_value()) {
				// FREE(remaining);
				// FF_EXIT;
				return -ENOENT;
			}

			// FREE(entries);
			// FREE(offsets);

			// int status = fat_read_dir(imgfd, &dir, &entries, &offsets, &count, NULL);
			int status = readDir(dir, entries, &offsets);
			if (status < 0) {
				// WARN(FATFINDH, "Couldn't read directory. Status: " BDR " (%s)", status, STRERR(status));
				// FREE(remaining);
				// FREE(offsets);
				// FREE(search);
				// FREE(entries);
				// FF_EXIT;
				return status;
			}

			// If we find any matches in the following for loop, this is set to 1.
			// If it stays 0, there were no (non-free) matches in the directory and the search failed.
			int found = 0;

			for (i = 0; i < count; i++) {
				DirEntry &entry = entries[i];
				char *fname = entry.name.str;
				if (search == fname && !isFree(entry)) {
					if (entry.isFile()) {
						if (at_end) {
							// We're at the end of the path and it's a file!
							// DBG(FATFINDH, "Returning file at the end.");
							// DBGF(FATFINDH, "  Name" DL " " BSTR DM " offset" DL " " BLR, entry->fname.str, offsets[i]);

							if (out)
								*out = entry;

							if (offset)
								*offset = offsets[i];
							// FF_EXIT;
							return status;
						}

						// At this point, we've found a file, but we're still trying to search it like a directory.
						// That's not valid because it's a directory, so an ENOTDIR error occurs.
						// WARN(FATFINDH, "Not a directory: " BSR, fname);
						// FF_EXIT;
						return -ENOTDIR;
					} else if (at_end) {
						// This is a directory at the end of the path. Success.
						// DBG(FATFINDH, "Returning directory at the end.");
						// DBG2(FATFINDH, "  Name:", entry->fname.str);

						if (out)
							*out = entry;

						if (offset)
							*offset = offsets[i];
						// FF_EXIT;
						return status;
					}

					// This is a directory, but we're not at the end yet. Search within this directory next.
					dir = entry;
					dir_offset = offsets[i];
					found = 1;
					break;
				}
			}

			done = remaining.empty();

			if (!found) {
				// None of the (non-free) entries in the directory matched.
				// DBG(FATFINDH, "Returning nothing.");
				// FF_EXIT;
				return -ENOENT;
			}
		} while (!done);

		// It shouldn't be possible to get here.
		// WARNS(FATFINDH, "Reached the end of the function " UDARR " " IDS("EIO"));
		// FF_EXIT;
		return -EIO;
	}

	DirEntry & DsFATDriver::getRoot(off_t *offset) {
		// If the root directory is already cached, we can simply return a pointer to the cached entry.
		if (root.startBlock != -1)
			return root;

		off_t start = superblock.startBlock * superblock.blockSize;
		if (offset)
			*offset = start;

		partition->read(&root, sizeof(DirEntry));
		return root;
	}


	int DsFATDriver::readDir(const DirEntry &dir, std::vector<DirEntry> &entries, std::vector<off_t> *offsets,
	                         int *first_index) {
// #ifndef DEBUG_DIRREAD
// 		METHOD_OFF(INDEX_DIRREAD);
// #endif

		// ENTER;
		// DBGFE("dir_read", IDS("Reading directory ") BSTR, dir->fname.str);

		if (dir.length == 0 || !dir.isDirectory()) {
			// If the directory is free or actually a file, it's not valid.
			// DR_EXIT;
			return -ENOTDIR;
		}

		// if (dir->length % sizeof(DirEntry) != 0)
		// 	WARN("dir_read", "Directory length " BDR " isn't a multiple of DirEntry size " BLR ".", dir.length,
		// 		sizeof(DirEntry));

		size_t count = dir.length / sizeof(DirEntry);
		entries.clear();
		entries.reserve(count);

		if (offsets) {
			offsets->clear();
			offsets->reserve(count);
		}

		std::vector<uint8_t> raw;
		size_t byte_c;

		int status = readFile(dir, raw, &byte_c);
		if (status < 0) {
			// DR_EXIT;
			return status;
		}

		char first_fname[DSFAT_PATH_MAX + 1];
		memcpy(first_fname, raw.data(), DSFAT_PATH_MAX);
		if (strcmp(first_fname, ".") == 0) {
			// The directory contains a "." entry, presumably in addition to a ".." entry.
			// This means there are two meta-entries before the actual entries.
			if (first_index)
				*first_index = 2;
		} else if (strcmp(first_fname, "..") == 0) {
			// The directory appears to contain just ".." and not "." (unless the order is reversed,
			// but that should never happen). This means there's just one meta-entry.
			if (first_index)
				*first_index = 1;
		}

		block_t block = dir.startBlock;
		int rem;

		for (size_t i = 0; i < count; i++) {
			memcpy(entries.data() + i, raw.data() + sizeof(DirEntry) * i, sizeof(DirEntry));
			const int entries_per_block = superblock.blockSize / sizeof(DirEntry);
			rem = i % entries_per_block;

			if (offsets)
				(*offsets)[i] = block * superblock.blockSize + rem * sizeof(DirEntry);

			if (rem == entries_per_block - 1)
				block = readFAT(block);
		}

		// DR_EXIT;
		return 0;
	}

	int DsFATDriver::readFile(const DirEntry &file, std::vector<uint8_t> &out, size_t *count) {
		// ENTER;
		// DBGFE(FILEREADH, IDS("Reading file ") BSTR " of length " BDR, file->fname.str, file->length);
		if (file.length == 0) {
			if (count)
				*count = 0;
			// EXIT;
			return 0;
		}

		uint32_t bs = superblock.blockSize;

		if (count)
			*count = file.length;

		out.clear();
		out.resize(file.length);
		uint8_t *ptr = out.data();
		uint32_t remaining = file.length;
		block_t block = file.startBlock;
		while (0 < remaining) {
			if (remaining <= bs) {
				checkBlock(block);
				// CHECKBLOCK("file_rd ", "Invalid block");
				// lseek(imgfd, block * bs, SEEK_SET);
				// CHECKS(FILEREADH, "Couldn't seek");
				// read(imgfd, ptr, remaining);
				// CHECKS(FILEREADH, "Couldn't read");
				partition->read(ptr, remaining, block * bs);
				ptr += remaining;
				remaining = 0;

				if (readFAT(block) != -2) {
					// The file should end here, but the file allocation table says there are still more blocks.
#ifndef SHRINK_DIRS
					bool shrink = false;
#else
					bool shrink = true;
#endif

					if (!shrink || !file.isDirectory()) {
						// WARN(FILEREADH, "%s " BSR " has no bytes left, but the file allocation table says more blocks are "
						// 	"allocated to it.", IS_FILE(*file)? "File" : "Directory", file->fname.str);
						// WARN(FILEREADH, SUB "remaining = " BDR DM " pcache.fat[" BDR "] = " BDR DM " bs = " BDR, remaining,
						// 	block, pcache.fat[block], bs);
					} else {
						// WARN(FILEREADH, "%s " BSR " has extra FAT blocks; trimming.", IS_FILE(*file)? "File" : "Directory",
						// 	file->fname.str);
						block_t nextblock = readFAT(block);
						writeFAT(-2, block);
						// DBGF(FILEREADH, BDR " ← -2", block);
						block_t shrinkblock = nextblock;
						for (;;) {
							nextblock = readFAT(shrinkblock);
							if (nextblock == -2) {
								// DBG(FILEREADH, "Finished shrinking (-2).");
								break;
							} else if (nextblock == 0) {
								// DBG(FILEREADH, "Finished shrinking (0).");
								break;
							} else if ((block_t) superblock.fatBlocks <= nextblock) {
								// WARN(FILEREADH, "FAT[" BDR "] = " BDR ", outside of FAT (" BDR " block%s)",
								// 	shrinkblock, nextblock, pcache.sb.fat_blocks, pcache.sb.fat_blocks == 1? "" : "s");
								break;
							}

							writeFAT(0, shrinkblock);
							++blocksFree;
							shrinkblock = nextblock;
						}
					}
				}
			} else {
				if (readFAT(block) == -2) {
					// There's still more data that should be remaining after this block, but
					// the file allocation table says the file doesn't continue past this block.
					// WARN(FILEREADH, "File still has " BDR " byte%s left, but the file allocation table doesn't have a next"
					// 	" block after " BDR ".", remaining, remaining == 1 ? "" : "s", block);

					// EXIT;
					return -EINVAL;
				} else {
					checkBlock(block);
					partition->read(ptr, bs, block * bs);
					ptr += bs;
					remaining -= bs;
					block = readFAT(block);
				}
			}
		}

		// EXIT;
		return 0;
	}

	block_t DsFATDriver::readFAT(size_t block_offset) {
		block_t out;
		partition->read(&out, sizeof(block_t), block_offset * sizeof(block_t));
		return out;
	}

	void DsFATDriver::writeFAT(block_t block, size_t block_offset) {
		partition->write(&block, sizeof(block_t), block_offset * sizeof(block_t));
	}

	bool DsFATDriver::isFree(const DirEntry &entry) {
		return entry.startBlock == 0 || readFAT(entry.startBlock) == 0;
	}

	bool DsFATDriver::checkBlock(block_t block) {
		if (block < 1) {
			printf("Invalid block: %d\n", block);
			for (;;);
			return false;
		}

		return true;
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
