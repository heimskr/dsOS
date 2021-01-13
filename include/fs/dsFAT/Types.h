#pragma once

#include <cstdint>
#include <ctime>

namespace DsOS::FS::DsFAT {
	using block_t = int32_t;
	using fd_t = uint64_t;

	constexpr size_t DSFAT_PATH_MAX = 255;
	constexpr size_t FD_MAX   = 128; // ???

	constexpr size_t PATHC_MAX = 1024;
	constexpr size_t FDC_MAX = 1024;

	struct Superblock {
		uint32_t magic;
		uint32_t blockCount;
		uint32_t fatBlocks;
		uint32_t blockSize;
		block_t startBlock; // the block containing the root directory
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
		Filename name;
		Times times;
		/** Length of the directory entry in bytes. For directories, 0 -> free. */
		size_t length;
		/** 0 if free, -1 if invalid. */
		block_t startBlock;
		FileType type;
		mode_t modes;
		char padding[16]; // update if DSFAT_PATH_MAX changes so that sizeof(DirEntry) is a multiple of 64

		bool isFile() const { return type == FileType::File; }
		bool isDirectory() const { return type == FileType::Directory; }
	} __attribute__((packed));
}