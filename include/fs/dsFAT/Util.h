#pragma once

#include <optional>
#include <string>

namespace DsOS::FS::DsFAT::Util {
	std::optional<std::string> pathFirst(std::string path, std::string *remainder);
}
