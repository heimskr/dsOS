#pragma once

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
}
