#pragma once

#include <cstdint>

namespace Thorn {
	using ThreadID = uint32_t;

	ThreadID currentThreadID();
	void currentThreadID(ThreadID);
}
