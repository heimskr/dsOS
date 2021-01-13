#pragma once

#include <cstdint>
#include <ctime>
#include <string.h>

namespace DsOS::FS::DsFAT {
	using block_t = int32_t;
	using fd_t = uint64_t;

	constexpr size_t DSFAT_PATH_MAX = 255;
	constexpr size_t FD_MAX   = 128; // ???

	constexpr size_t PATHC_MAX = 1024;
	constexpr size_t FDC_MAX = 1024;

	struct Superblock {
		uint32_t magic;
		size_t blockCount;
		uint32_t fatBlocks;
		uint32_t blockSize;
		/** The block containing the root directory. */
		block_t startBlock;
	};

	struct Times {
		time_t created;
		time_t modified;
		time_t accessed;
	};

	union Filename {
		char str[DSFAT_PATH_MAX + 1] = {0};
		uint64_t longs[sizeof(str) / sizeof(uint64_t)];
	};

	enum class FileType: int {File, Directory};

	struct DirEntry {
		Filename name = {0};
		Times times;
		/** Length of the directory entry in bytes. For directories, 0 -> free. */
		size_t length = 0;
		/** 0 if free, -1 if invalid. */
		block_t startBlock = {-1};
		FileType type;
		mode_t modes = 0;
		char padding[20] = {0}; // update if DSFAT_PATH_MAX changes so that sizeof(DirEntry) is a multiple of 64

		bool isFile() const { return type == FileType::File; }
		bool isDirectory() const { return type == FileType::Directory; }
		void reset() {
			memset(name.str, 0, sizeof(name));
			times = {0, 0, 0};
			length = 0;
			startBlock = -1;
			type = FileType::File;
			modes = 0;
		}
	} __attribute__((packed));

	static_assert(sizeof(DirEntry) % 64 == 0);
}