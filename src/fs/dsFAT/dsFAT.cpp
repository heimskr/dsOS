#include <cerrno>
#include <string>

#include "fs/dsFAT/dsFAT.h"
#include "fs/dsFAT/Types.h"
#include "fs/dsFAT/Util.h"
#include "lib/printf.h"
#include "memory/Memory.h"
#include "DsUtil.h"

#define FD_VALID(fd) ((fd) != UINT64_MAX)

namespace DsOS::FS::DsFAT {
	char DsFATDriver::nothing[sizeof(DirEntry)] = {0};

	DsFATDriver::DsFATDriver(Partition *partition_): Driver(partition_) {
		root.startBlock = -1;
		readSuperblock(superblock);
	}

	int DsFATDriver::readSuperblock(Superblock &out) {
		int status = partition->read(&out, sizeof(Superblock), 0);
		if (status != 0)
			printf("[DsFATDriver::readSuperblock] Reading failed: %s\n", strerror(status));
		return -status;
	}

	void DsFATDriver::error(const std::string &err) {
		printf("Error: %s\n", err.c_str());
		for (;;);
	}

	int DsFATDriver::find(fd_t fd, const char *path, DirEntry *out, DirEntry **outptr, off_t *offset, bool get_parent,
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

				if (outptr)
					*outptr = &pc_entry->entry;
				else if (out)
					*out = pc_entry->entry;

				// DBG(FATFINDH, "Returning from cache."); FF_EXIT;
				return status;
			}
		}

		if (strcmp(path, "/") == 0 || strlen(path) == 0) {
			// For the special case of "/" or "" (maybe that last one should be invalid...), return the root directory.
			DirEntry &rootdir = getRoot(offset);
			if (outptr)
				*outptr = &rootdir;
			else if (out)
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
				if (outptr)
					*outptr = &dir;
				else if (out)
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

			int status = readDir(dir, entries, &offsets);
			count = entries.size();
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

							if (outptr)
								*outptr = &entry;
							else if (out)
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

						if (outptr)
							*outptr = &entry;
						else if (out)
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

	size_t DsFATDriver::forget(block_t start) {
		// ENTER;

		// DBGNE(FORGETH, "Forgetting", start);

		int64_t next;
		block_t block = start;
		size_t removed = 0;
		for (;;) {
			next = readFAT(block);
			if (next == -2) {
				// DBGFE(FORGETH, "Freeing " BLR " (next == -2)", block);
				writeFAT(0, block);
				removed++;
				break;
			} else if (0 < next) {
				// DBGFE(FORGETH, "Freeing " BLR " (0 < next)", block);
				writeFAT(0, block);
				removed++;
				block = next;
			} else
				break;
		}

		blocksFree += removed;
		// save(&superblock, pcache.fat);
		// EXIT;
		return removed;
	}

	int DsFATDriver::writeEntry(const DirEntry &dir, off_t offset) {
		// HELLO(dir->fname.str);
		// ENTER;
		// DBGF(WRENTRYH, "Writing directory entry at offset " BLR " (" BLR DMS BLR DL BLR ") with filename " BSTR DM " length " BUR
		// 	DM " start block " BDR " (next: " BDR ")",
		// 	offset, offset / superblock.blockSize, (offset % superblock.blockSize) / sizeof(dir_t),
		// 	(offset % superblock.blockSize) % sizeof(dir_t),  dir->fname.str, dir->length, dir->start_block,
		// 	pcache.fat[dir->start_block]);

		// CHECKSEEK(WRENTRYH, "Invalid offset:", offset);
		// lseek(imgfd, offset, SEEK_SET);
		// IFERRNOXC(WARN(WRENTRYH, "lseek() failed " UDARR " " DSR, strerror(errno)));

		int status = partition->write(&dir, sizeof(DirEntry), offset);
		if (status != 0) {
			printf("[DsFATDriver::writeEntry] Writing failed: %s\n", strerror(status));
			return -status;
		}
		// IFERRNOXC(WARN(WRENTRYH, "write() failed " UDARR " " DSR, strerror(errno)));

		if (offset == superblock.startBlock * superblock.blockSize) {
			// If we're writing to "." in the root, update ".." as well.
			// We need to make a copy of the entry so we can change its filename to "..".
			DirEntry dir_cpy = dir;
			memset(dir_cpy.name.str, 0, sizeof(dir_cpy.name.str));
			dir_cpy.name.str[0] = dir_cpy.name.str[1] = '.';
			// write(imgfd, &dir_cpy, sizeof(dir_t));
			partition->write(&dir_cpy, sizeof(DirEntry), offset + sizeof(DirEntry));
			if (status != 0) {
				printf("[DsFATDriver::writeEntry] Writing failed: %s\n", strerror(status));
				return -status;
			}
			// IFERRNOXC(WARN(WRENTRYH, "write() failed " UDARR " " DSR, strerror(errno)));
		}

		if (dir.startBlock == superblock.startBlock) {
			// DBGF(WRENTRYH, "Writing root (" BDR SUDARR BDR "); dir %c= rootptr.", root.length, dir.length, &dir == &pcache.root? '=' : '!');
			if (&dir != &root)
				root.length = dir.length;
		}

		// If fat_write_entry() is called, it's probably because something about the entry was recently changed.
		// If the length changed, this might invalidate a corresponding pcache entry.
		// We're assuming the offset is still valid.
		PathCacheEntry *found = pathCache.find(offset);
		if (found && found->entry.length != dir.length) {
			// DBGF(WRENTRYH, "Updating " IMS("pcache") " entry length for " BSR DLS BDR SUDARR BDR, found->path, found->entry.length, dir->length);
			found->entry.length = dir.length;
		}

		// EXIT;
		return 0;
	}

	DirEntry & DsFATDriver::getRoot(off_t *offset) {
		// If the root directory is already cached, we can simply return a pointer to the cached entry.
		if (root.startBlock != -1)
			return root;

		off_t start = superblock.startBlock * superblock.blockSize;
		if (offset)
			*offset = start;

		partition->read(&root, sizeof(DirEntry), start);
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
								// 	shrinkblock, nextblock, superblock.fat_blocks, superblock.fat_blocks == 1? "" : "s");
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

	int DsFATDriver::newFile(const char *path, uint32_t length, FileType type, const Times *times, DirEntry **dir_out,
	                         off_t *offset_out, DirEntry **parent_dir_out, off_t *parent_offset_out, bool noalloc) {
		// HELLO(path);
// #ifndef DEBUG_NEWFILE
// 		METHOD_OFF(INDEX_NEWFILE);
// #endif
		// ENTER;
		// DBG2(NEWFILEH, "Trying to create new file at", path);

		DirEntry *parent = nullptr;
		off_t parent_offset = 0;
		std::string last_name;
		int status = find(-1, path, nullptr, &parent, &parent_offset, true, &last_name);
		bool parent_is_new = status == 1;

		if (status < 0 && status != -ENOENT) {
			// It's fine if we get ENOENT. The existence of a file shouldn't be a prerequisite for its creation.
			printf("[DsFATDriver::newFile] find failed: %s\n", strerror(-status));
			// NF_EXIT;
			return status;
		}

		// First, check the filename length.
		const size_t ln_length = last_name.size();
		if (DSFAT_PATH_MAX < ln_length) {
			// If it's too long, we can give up early.
			printf("[DsFATDriver::newFile] Name too long (%lu chars)\n", ln_length);
			// WARN(NEWFILEH, "Name too long: " BSR " " UDARR " " IDS("ENAMETOOLONG"), last_name);
			// FREE(last_name);
			// FREE(parent);
			// NF_EXIT;
			return -ENAMETOOLONG;
		}

		DirEntry newfile = {
			.times = (times == NULL? Times(0, 0, 0) : *times), // TODO: implement time
			.length = length,
			.type = type,
		};

		block_t free_block = findFreeBlock();
		if (noalloc) {
			// We provide an option not to allocate space for the file. This is helpful for fat_rename, when we just
			// need to find an offset where an existing directory entry can be moved to. It's possibly okay if no
			// free block is available if the parent directory has enough space for another entry.
			newfile.startBlock = 0;
		} else {
			// We'll need at least one free block to store the new file in so we can
			// store the starting block in the directory entry we'll create soon.
			if (free_block == -1) {
				// If we don't have one, we'll complain about having no space left and give up.
				// WARNS(NEWFILEH, "No free block " UDARR " " IDS("ENOSPC"));
				// FREE(last_name);
				// FREE(parent);
				// NF_EXIT;
				return -ENOSPC;
			}

			// SUCC(NEWFILEH, "Allocated " BSR " at block " BDR ".", path, free_block);

			// Allocate the first block, decrement the free blocks count and flush the cache to disk.
			// pcache.fat[free_block] = -2;
			writeFAT(-2, free_block);
			--blocksFree;
			newfile.startBlock = free_block;

			// There's not a point in copying the name to the entry if this function
			// is being called just to get an offset to move an existing entry to.
			memcpy(newfile.name.str, last_name.c_str(), ln_length);
		}

		// FREE(last_name);
		block_t old_free_block = free_block;

		// Read the directory to check whether there's a freed entry we can recycle. This can save us a lot of pain.
		std::vector<DirEntry> entries;
		std::vector<off_t> offsets;
		int first_index;
		status = readDir(*parent, entries, &offsets, &first_index);
		size_t count = entries.size();

		// Check all the directory entries except the initial meta-entries.
		off_t offset = -1;
		size_t offset_index = -1;
		for (size_t i = (size_t) first_index; i < count; ++i) {
			if (isFree(entries[i])) {
				// We found one! Store its offset in `offset` to replace its previous value of -1.
				offset = offsets[i];
				offset_index = i;
				// SUCC(NEWFILEH, ICS("Found") " freed entry at offset " BLR ".", offset);
				break;
			}
		}

		// FREE(entries);
		// FREE(offsets);

		const size_t bs = superblock.blockSize;
		const uint64_t block_c = DsOS::Util::updiv(static_cast<size_t>(length), bs);

		// There are four different scenarios that we have to accommodate when we want to add a new directory entry.
		// The first and easiest is when the parent directory has a slot that used to contain an entry but was later freed.
		// The second is when the parent directory is less than a block long, so we take its first free slot.
		// The third is when the parent directory spans multiple blocks and is fully block-aligned, so we have to go through
		// the effort of adding a new block to it. The fourth is like the third, except the parent directory isn't
		// block-aligned so we don't have to expend a lot of effort, much like the second scenario.

		int increase_parent_length = 1;

		if (offset != -1) {
			// Scenario one: we found a free entry earlier. Way too easy.
			// CDBGS(A_CYAN, NEWFILEH, "Scenario one.");
			status = writeEntry(newfile, offset);
			if (status < 0) {
				printf("[DsFATDriver::newFile] Couldn't add entry to parent directory: %s\n", strerror(-status));
				// NF_EXIT;
				return status;
			}

			// DBGF(NEWFILEH, "[S1] parent->length" SDEQ BUR DMS "offset_index" SDEQ BULR DMS "parent->length / sizeof(dir_t)" SDEQ BULR DMS "%s", parent->length, offset_index, parent->length / sizeof(dir_t), parent == &pcache.root? "==" : "!=");
			if (offset_index < parent->length / sizeof(DirEntry)) {
				// Don't increase the length if it already extends past this entry.
				increase_parent_length = 0;
			}
		} else if (parent->length <= bs - sizeof(DirEntry)) {
			// Scenario two: the parent directory has free space in its first block, which is also pretty easy to deal with.
			// CDBGS(A_CYAN, NEWFILEH, "Scenario two.");

			if (!noalloc && !hasFree(block_c)) {
				// If we're allocating space for the new file, we need enough blocks to hold it.
				// If we don't have enough blocks, we should stop now before we make any changes to the filesystem.
				// WARN(NEWFILEH, "No free block " UDARR " " DSR, "ENOSPC");
				writeFAT(0, old_free_block);
				// pcache.fat[old_free_block] = 0;
				// FREE(parent);
				// NF_EXIT;
				return -ENOSPC;
			}

			// The offset of the free space is the sum of the parent's starting offset and its length.
			// Try to write the entry to it.
			offset = parent->startBlock * bs + parent->length;
			status = writeEntry(newfile, offset);
			if (status < 0) {
				printf("[DsFATDriver::newFile] Couldn't add entry to parent directory: %s\n", strerror(-status));
				// NF_EXIT;
				return status;
			}
		} else {
			// Scenarios three and four: the parent directory spans multiple blocks.
			// Four is mercifully simple, three less so.

			// Skip to the last block; we don't need to read or change anything in the earlier blocks.
			size_t remaining = parent->length;
			block_t block = parent->startBlock;
			printf("[DsFATDriver::newFile] Parent start block: %ld\n", block);
			int skipped = 0;
			while (bs < remaining) {
				block = readFAT(block);
				remaining -= bs;
				if (++skipped <= NEWFILE_SKIP_MAX)
					printf("[DsFATDriver::newFile] Skipping to %ld\n", block);
			}

			if (NEWFILE_SKIP_MAX < skipped)
				printf("[DsFATDriver::newFile]   ... %d more\n", skipped);

			// DBGF(NEWFILEH, "bs - sizeof(dir_t) < remaining  " UDBARR "  " BLR " - " BLR " < " BLR "  " UDBARR "  " BLR " < " BLR, bs, sizeof(dir_t), remaining, bs - sizeof(dir_t), remaining);

			if (bs - sizeof(DirEntry) < remaining) {
				// Scenario three, the worst one: there isn't enough free space left in the
				// parent directory to fit in another entry, so we have to add another block.
				// CDBGS(A_CYAN, NEWFILEH, "Scenario three.");

				bool nospc = false;

				if (!noalloc && !hasFree(2 + block_c)) {
					// We need to make sure we have at least two free blocks: one for the expansion of the parent
					// directory and another for the new directory entry. We also need more free blocks to contain the
					// file, but that's mostly handled by the for loop below.
					// TODO: is it actually 1 + block_c?
					writeFAT(0, old_free_block);
					nospc = true;
				} else if (noalloc && !hasFree(1)) {
					// If we don't need to allocate space for the new file,
					// we need only one extra block for the parent directory.
					nospc = true;
				}

				if (nospc) {
					// If we don't have enough free blocks, the operation fails due to lack of space.
					// WARN(NEWFILEH, "No free block (need " BLR ") " UDARR " " DSR, 2 + block_c, "ENOSPC");
					// NF_EXIT;
					return -ENOSPC;
				}

				// We'll take the value from the free block we found earlier to use as the next block in the parent
				// directory.
				writeFAT(old_free_block, block);
				block = old_free_block;

				if (!noalloc) {
					// If we need to allocate space for the new file, we now try to find
					// another free block to use as the new file's start block.
					free_block = findFreeBlock();
					if (free_block == -1) {
						printf("[DsFATDriver::newFile] No free block -> ENOSPC\n");
						writeFAT(0, old_free_block);
						// NF_EXIT;
						return -ENOSPC;
					}

					// Decrease the free block count and assign the free block as the new file's starting block.
					--blocksFree;
					newfile.startBlock = free_block;
				}

				// Write the directory entry to the block we allocated for expanding the parent directory.
				offset = block * bs;
				status = writeEntry(newfile, offset);
				if (status < 0) {
					printf("[DsFATDriver::newFile] Couldn't add entry to parent directory: %s\n", strerror(-status));
					// NF_EXIT;
					return status;
				}
			} else {
				// Scenario four: there's enough space in the parent directory to add the new entry. Nice.
				// CDBGS(A_CYAN, NEWFILEH, "Scenario four.");

				offset = block * bs + remaining;
				status = writeEntry(newfile, offset);
				if (status < 0) {
					printf("[DsFATDriver::newFile] Couldn't add entry to parent directory: %s\n", strerror(-status));
					// NF_EXIT;
					return status;
				}
			}
		}

		if (increase_parent_length) {
			// Increase the size of the parent directory and write it back to its original offset.
			printf("[DsFATDriver::newFile] Increasing parent length.\n");
			// DBGFE(NEWFILEH, "Parent length" DLS BDR SUDARR BDR, parent->length, (uint32_t) (parent->length+sizeof(dir_t)));
			parent->length += sizeof(DirEntry);
			status = writeEntry(*parent, parent_offset);
			if (status < 0) {
				printf("[DsFATDriver::newFile] Couldn't write the parent directory to disk: %s\n", strerror(-status));
				// NF_EXIT;
				return status;
			}
		}

		if (!noalloc) {
			// If the file is more than one block in length, we need to allocate more entries in the file allocation table.
			block_t block = newfile.startBlock;
			for (auto size_left = length; bs < size_left; size_left -= bs) {
				block_t another_free_block = findFreeBlock();
				if (another_free_block == -1) {
					writeFAT(-2, block);
					blocksFree = -1;
					writeFAT(0, old_free_block);
					printf("[DsFATDriver::newFile] No free block -> ENOSPC\n");
					countFree();
					// fat_save(imgfd, &superblock, pcache.fat);
					// NF_EXIT;
					return -ENOSPC;
				}

				--blocksFree;
				writeFAT(another_free_block, block);
				block = another_free_block;
			}

			writeFAT(-2, block);

			// Attempt to insert the new item into the pcache.
			// DBGF(NEWFILEH, "About to insert into " IMS("pcache") DLS BSTR DMS "offset" DLS BLR, newfile.fname.str, offset);
			PathCacheEntry *item;
			PCInsertStatus insert_status = pathCache.insert(path, newfile, offset, &item);
			// DBG(NEWFILEH, "Inserted.");
			if (insert_status != PCInsertStatus::Success && insert_status != PCInsertStatus::Overwritten) {
				// Inserting a pcache entry should always work. If it doesn't, there's a bug somewhere.
				printf("[DsFATDriver::newFile] pc_insert failed (%d)\n", insert_status);
				for (;;);
				return -666;
			} else {
				if (noalloc)
					pathCache.erase(*item);
				if (dir_out)
					*dir_out = &item->entry;
			}

			// if (dir_out != NULL) {
		} else if (dir_out) {
			overflow[overflowIndex] = newfile;
			*dir_out = &overflow[overflowIndex];
			overflowIndex = (overflowIndex + 1) % OVERFLOW_MAX;
		}

		// fat_save(imgfd, &superblock, pcache.fat);

		if (offset_out)
			*offset_out = offset;

		if (parent_dir_out)
			*parent_dir_out = parent;

		if (parent_offset_out)
			*parent_offset_out = parent_offset;

		if (offset_out)
			*offset_out = offset;
		if (parent_dir_out)
			*parent_dir_out = parent;
		if (parent_offset_out)
			*parent_offset_out = parent_offset;

		// SUCCSE(NEWFILEH, "Created new file.");
		// NF_EXIT;
		return parent_is_new;
	}

	int DsFATDriver::remove(const char *path, bool remove_pentry) {
		// DBGF(REMOVEH, "Removing " BSR " from the filesystem %s from " IMS("pcache") ".",
		// 	path, remove_pentry? "and" : "but not");

		DirEntry *found;
		off_t offset;

		// DBG_OFFE();
		int status = find(-1, path, nullptr, &found, &offset, false, nullptr);
		// DBG_ONE();
		if (status < 0) {
			printf("[DsFATDriver::remove] find failed: %s\n", strerror(-status));
			return status;
		}

		// DBGF(REMOVEH, "Offset" DL " " BLR DM " start" DL " " BDR DM " next block" DL " " BDR,
		// 	offset, found->start_block, pcache.fat[found->start_block]);

		forget(found->startBlock);
		found->startBlock = 0;
		writeEntry(*found, offset);
		// fat_save(imgfd, &superblock, pcache.fat);

		if (remove_pentry)
			pathCache.erase(path);

		return 0;
	}

	block_t DsFATDriver::findFreeBlock() {
		auto block_c = superblock.blockCount;
		for (decltype(block_c) i = 0; i < block_c; i++)
			if (readFAT(i) == 0) // TODO: cache FAT
				return i;
		return -1;
	}

	void DsFATDriver::initFAT(size_t table_size, size_t block_size) {
		size_t written = 0;
		// These blocks point to the FAT, so they're not valid regions to write data.
		writeMany((block_t) -1, table_size + 1);
		written += table_size + 1;
		writeMany((block_t) -2, 1);
		++written;
		// Might be sensitize to sizeof(block_t).
		writeMany((block_t) 0, Util::blocks2words(table_size, block_size) - written);
	}

	block_t DsFATDriver::readFAT(size_t block_offset) {
		block_t out;
		int status = partition->read(&out, sizeof(block_t), block_offset * sizeof(block_t));
		if (status != 0)
			printf("[DsFATDriver::readFAT] Reading failed: %s\n", strerror(status));
		return out;
	}

	int DsFATDriver::writeFAT(block_t block, size_t block_offset) {
		int status = partition->write(&block, sizeof(block_t), block_offset * sizeof(block_t));
		if (status != 0) {
			printf("[DsFATDriver::writeFAT] Writing failed: %s\n", strerror(status));
			return -status;
		}
		return 0;
	}

	void DsFATDriver::initData(size_t block_count, size_t table_size) {
		root.reset();
		root.name.str[0] = '.';
		root.length = 2 * sizeof(DirEntry);
		root.startBlock = table_size + 1;
		root.type = FileType::Directory;
		// fat_update_times(&root, now);
		// writeEntry(root,
		// auto write_entry = [&](const DirEntry &entry) {
		// 	for (size_t i = 0; i < sizeof(dir.name.words) / sizeof(dir.name.words[0]); ++i)
		// 		write(dir.name.words[i]);
		// 	write(dir.times.created);
		// 	write(dir.times.modified);
		// 	write(dir.times.accessed);
		// 	write(dir.length);
		// 	write(dir.start_block);
		// 	write(dir.flags);
		// 	write(static_cast<int>(0), 5);
		// };
		write(root);
		root.name.str[1] = '.';
		write(root);
	}

	bool DsFATDriver::hasFree(const size_t count) {
		size_t scanned = 0;
		size_t block_c = superblock.blockCount;
		for (size_t i = 0; i < block_c; i++)
			if (readFAT(i) == 0 && count <= ++scanned)
				return true;
		return false;
	}

	bool DsFATDriver::isFree(const DirEntry &entry) {
		return entry.startBlock == 0 || readFAT(entry.startBlock) == 0;
	}

	ssize_t DsFATDriver::countFree() {
		if (0 <= blocksFree)
			return blocksFree;

		blocksFree = 0;
		for (int i = superblock.blockCount - 1; 0 <= i; --i)
			if (readFAT(i) == 0)
				++blocksFree;
		return blocksFree;
	}

	bool DsFATDriver::checkBlock(block_t block) {
		if (block < 1) {
			printf("Invalid block: %d\n", block);
			for (;;);
			return false;
		}

		return true;
	}

	size_t DsFATDriver::tableSize(size_t block_count, size_t block_size) {
		return block_count < block_size / sizeof(block_t)?
			1 : DsOS::Util::updiv(block_count, block_size / sizeof(block_t));
	}

	int DsFATDriver::rename(const char *srcpath, const char *destpath) {
		// HELLO(srcpath);
		// DBGL;
		// DBGF(RENAMEH, CMETHOD("rename") BSTR " " UARR " " BSTR, srcpath, destpath);

		DirEntry *dest_entry, *src_entry;
		off_t dest_offset, src_offset;
		int status;

		// It's okay if the destination doesn't exist, but the source is required to exist.
		if ((status = find(-1, srcpath, nullptr, &src_entry, &src_offset, false, nullptr)) < 0) {
			printf("[DsFATDriver::rename] find failed for old path: %s\n", strerror(-status));
			return status;
		}

		// Start by removing the source from the pcache.
		pathCache.erase(src_offset);

		status = find(-1, destpath, nullptr, &dest_entry, &dest_offset, false, nullptr);
		// DBGN(RENAMEH, "fat_find status:", status);
		if (status < 0 && status != -ENOENT) {
			printf("[DsFATDriver::rename] find failed for new path: %s\n", strerror(-status));
			return status;
		}

		if (0 <= status) {
			// DBGF(RENAMEH, IUS("Destination " BSR " already exists."), dest_entry->fname.str);
			// Because the destination already exists, we need to unlink it to free up its space.
			// The offset of its directory entry will then be available for us to use.
			// We don't have to worry about not having enough space in this case; in fact,
			// we're actually freeing up at least one block.
			status = remove(destpath, true);
			if (status < 0) {
				printf("[DsFATDriver::rename] Couldn't unlink target: %s\n", strerror(-status));
				return status;
			}

			// fat_remove would normally remove the corresponding pcache entry,
			// but it uses string comparisons. We have the offset, so we can do
			// it faster if we search by offset instead.
			pathCache.erase(dest_offset);
		} else if (status == -ENOENT) {
			// DBGFE(RENAMEH, IUS("Destination " BSR " doesn't exist."), destpath);
			// If the destination doesn't exist, we need to try to add another directory entry to the destination directory.

			DirEntry *parent_entry;
			off_t  parent_offset;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
			status = newFile(destpath, 0, src_entry->isDirectory()? FileType::Directory : FileType::File,
			                 &src_entry->times, &dest_entry, &dest_offset, &parent_entry, &parent_offset, 1);
#pragma GCC diagnostic pop
			if (status < 0) {
				printf("[DsFATDriver::rename] Couldn't create new directory entry: %s\n", strerror(-status));
				return status;
			}
			// DBGF(RENAMEH, BLR SUDARR BLR, src_offset, dest_offset);
			// FREE(dest_entry);
			// if (status == 1) {
			// 	FREE(parent_entry);
			// }

			// We now know the offset of a free directory entry in the destination's parent directory.
			// (We just created one, after all.) Now we copy over the source entry before freeing it.
		}

		// Zero out the source entry's filename, then copy in the basename of the destination.
		std::optional<std::string> destbase = Util::pathLast(destpath);
		if (!destbase.has_value()) {
			printf("[DsFATDriver::rename] destbase has no value!\n");
			return -EINVAL;
		}

		memset(src_entry->name.str, 0, DSFAT_PATH_MAX + 1);
		strncpy(src_entry->name.str, destbase->c_str(), DSFAT_PATH_MAX);

		// Once we've ensured the destination doesn't exist or no longer exists,
		// we move the source's directory entry to the destination's offset.
		status = partition->write(src_entry, sizeof(DirEntry), dest_offset);
		if (status != 0) {
			printf("[DsFATDriver::rename] Writing failed: %s\n", strerror(status));
			return -status;
		}

		// Now we need to remove the source entry's original directory entry from the disk image.
		status = partition->write(nothing, sizeof(DirEntry), src_offset);
		if (status != 0) {
			printf("[DsFATDriver::rename] Writing failed: %s\n", strerror(status));
			return -status;
		}

		// Finally, we add the newly moved file to the pcache.
		// DBG(RENAMEH, "Adding to " IMS("pcache") ".");
		pathCache.insert(destpath, *src_entry, dest_offset, nullptr);
		// SUCC(RENAMEH, "Moved " BSR " to " BSR ".", srcpath, destpath);
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

	bool DsFATDriver::make(uint32_t block_size) {
		int status = partition->clear();
		if (status != 0) {
			printf("[DsFATDriver::make] Clearing partition failed: %s\n", strerror(status));
			return false;
		}

		const size_t block_count = partition->length / block_size;

		if (block_count < MINBLOCKS) {
			printf("[DsFATDriver::make] Number of blocks for partition is too small: %lu\n", block_count);
			return false;
		}

		if (block_size % sizeof(DirEntry)) {
			// The block size must be a multiple of the size of a directory entry because it must be possible to fill a
			// block with directory entries without any space left over.
			printf("[DsFATDriver::make] Block size isn't a multiple of %lu.\n", sizeof(DirEntry));
			return false;
		}

		if (block_size < 2 * sizeof(DirEntry)) {
			printf("[DsFATDriver::make] Block size must be able to hold at least two directory entries.\n");
			return false;
		}

		if (block_size < sizeof(Superblock)) {
			printf("[DsFATDriver::make] Block size isn't large enough to contain a superblock (%luB).\n",
				sizeof(Superblock));
			return false;
		}

		const size_t table_size = tableSize(block_count, block_size);

		if (UINT32_MAX <= table_size) {
			printf("[DsFATDriver::make] Table size too large: %u\n", table_size);
			return false;
		}

		superblock = {
			.magic = MAGIC,
			.blockCount = block_count,
			.fatBlocks = static_cast<uint32_t>(table_size),
			.blockSize = block_size,
			.startBlock = static_cast<block_t>(table_size + 1)
		};

		writeOffset = 0;
		write(superblock);
		initFAT(table_size, block_size);
		initData(block_count, table_size);
		return true;
	}
}
