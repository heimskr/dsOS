#pragma once

#include <cstdint>
#include <ctime>

namespace DsOS::FS::DsFAT {
	using block_t = int32_t;
	using fd_t = uint64_t;

	constexpr size_t PATH_MAX = 256;
	constexpr size_t FD_MAX   = 128; // ???

	constexpr size_t PATHC_MAX = 1024;
	constexpr size_t FDC_MAX = 1024;

	struct Superblock {
		uint32_t magic;
		uint32_t block_count;
		uint32_t fat_blocks;
		uint32_t block_size;
		block_t start_block; // the block containing the root directory
	};

	struct Times {
		time_t created;
		time_t modified;
		time_t accessed;
	};

	union Filename {
		char str[PATH_MAX] = {0};
		uint64_t longs[sizeof(str) / sizeof(uint64_t)];
	};

	enum class FileType: int {File, Directory};

	struct DirEntry {
		Filename name;
		Times times;
		size_t length; // in bytes. for directories, 0 -> free
		block_t startBlock; // 0 if free, -1 if invalid
		FileType type;
		mode_t modes;
		char padding[16]; // update if PATH_MAX changes so that sizeof(DirEntry) is a multiple of 64
	} __attribute__((packed));
}