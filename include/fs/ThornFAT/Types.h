#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <string.h>
#include "Defs.h"

namespace Thorn::FS::ThornFAT {
	using block_t = int32_t;
	using fd_t = uint64_t;

	constexpr size_t THORNFAT_PATH_MAX = 255;
	constexpr size_t FD_MAX = 128; // ???

	constexpr size_t PATHC_MAX = 1024;
	constexpr size_t FDC_MAX   = 1024;

	constexpr block_t UNUSABLE = -1;
	constexpr block_t FINAL    = -2;

	struct Superblock {
		uint32_t magic;
		size_t blockCount;
		uint32_t fatBlocks;
		uint32_t blockSize;
		/** The block containing the root directory. */
		block_t startBlock;
		void print() const;
	} __attribute__((packed));

	struct Times {
		long created  = 0;
		long modified = 0;
		long accessed = 0;
		Times() = default;
		Times(long created_, long modified_, long accessed_):
			created(created_), modified(modified_), accessed(accessed_) {}
	};

	union Filename {
		char str[THORNFAT_PATH_MAX + 1] = {0};
		uint64_t longs[sizeof(str) / sizeof(uint64_t)];
	};

	enum class FileType: int {File, Directory};

	struct DirEntry {
		Filename name = {{0}};
		Times times;
		/** Length of the directory entry in bytes. For directories, 0 -> free. */
		size_t length = 0;
		/** 0 if free, -1 if invalid. */
		block_t startBlock = {-1};
		FileType type = FileType::File;
		mode_t modes = 0;
		char padding[16] = {0}; // update if THORNFAT_PATH_MAX changes so that sizeof(DirEntry) is a multiple of 64

		DirEntry() = default;
		DirEntry(const Times &, size_t, FileType);
		DirEntry(const DirEntry &);
		DirEntry & operator=(const DirEntry &);
		bool isFile() const { return type == FileType::File; }
		bool isDirectory() const { return type == FileType::Directory; }
		void reset();
		operator std::string() const;
		void print() const;
	};

	static_assert(sizeof(DirEntry) % 64 == 0);
}
