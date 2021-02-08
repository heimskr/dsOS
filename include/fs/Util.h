#pragma once

#include <string>

namespace Thorn::FS {
	std::string simplifyPath(std::string cwd, std::string);
	std::string simplifyPath(std::string);
}
