#pragma once

#include "Defs.h"
#include "fs/dsFAT/Types.h"

#include <optional>
#include <string>

namespace DsOS::FS::DsFAT::Util {
	std::optional<std::string> pathFirst(std::string path, std::string *remainder);
	/**
	 * Returns the last component of a path.
	 * Examples: foo => foo, /foo => foo, /foo/ => foo, /foo/bar => bar, /foo/bar/ => bar
	 * basename() from libgen.h is similar to this function, but I wanted a function that allocates memory automatically
	 * and that doesn't return "/" or "."
	 */
	std::optional<std::string> pathLast(const char *);

	std::optional<std::string> pathParent(const char *);

	inline size_t blocks2words(const size_t blocks, const size_t block_size) {
		// The blocksize is in bytes, so we divide it by sizeof(block) to get the number of words.
		// Multiply that by the number of blocks to get how many words would be taken up by that many blocks.
		return blocks * (size_t) (block_size * sizeof(char) / sizeof(block_t));
	}
}
